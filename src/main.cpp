#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <print>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>
#include "aes_xts.hpp"
#include "cross_platform.hpp"
#include "progress_bars.hpp"
#include "mio.hpp"

static const uint32_t SECTOR_SIZE = 512;

[[maybe_unused]] static inline void print_hex(const char * label, const std::vector<uint8_t> &bytes) {
   std::print("{}: ", label);
    for (auto &hex: bytes) {
        std::print("{:02X}", hex);
    }
    std::println("");
}

uint8_t hex_to_nibble(char input) {
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw std::invalid_argument(std::format("Invalid hex digit %c", input));

  // will never get here, but compiler complains about the lack of return
  return 0;
}

std::vector<uint8_t> hex_to_bytes(const char *hex) {
    std::vector<uint8_t> bytes;

    size_t len = strlen(hex);
    if (len % 2 != 0) {
        return bytes;
    }

    for (size_t i = 0; i < len; i += 2) {
        bytes.push_back(hex_to_nibble(hex[i]) << 4 | hex_to_nibble(hex[i+1]));
    }

    return bytes;
}

int create_sparse(const char *file_path, uint64_t len) {
    std::ofstream ofs(file_path, std::ios::binary | std::ios::out);
    ofs.seekp(len-1);
    ofs.write("\x00", 1);
    return ofs.rdstate(); // good is 0
}

bool file_exists(const char *file_path) {
    return std::ifstream(file_path).good();
}

int main(int argc, const char *argv[]) {
    std::error_code error;

    if (argc < 6) {
        std::filesystem::path program = argv[0];
        std::println("Usage: {} <XTS_KEY> <XTS_TWEAK> <IV_OFFSET> <in_path> <out_path>", program.filename().string());
        std::println("For details see:");
        std::println("\thttps://www.psdevwiki.com/ps4/Mounting_HDD_in_Linux");
        std::println("\thttps://www.psdevwiki.com/ps4/Partitions");
        return 1;
    }

    auto xts_key = hex_to_bytes(argv[1]);
    auto xts_tweak = hex_to_bytes(argv[2]);
    uint64_t iv_offset = strtoull(argv[3], NULL, 10);
    const char *in_filepath = argv[4];
    const char *out_filepath = argv[5];
    
    if (xts_key.size() != 16) {
        std::println("XTS_KEY must be 16 bytes");
        return -1;
    }

    if (xts_tweak.size() != 16) {
        std::println("XTS_TWEAK must be 16 bytes");
        return -1;
    }

    if (file_exists(out_filepath)) {
        std::println("Output file '{}' exists, please remove.", out_filepath);
        return -1;
    }

    // mmap input file read only with shared semantics
    mio::shared_mmap_source source = mio::make_mmap_source(in_filepath, 0, mio::map_entire_file, error);
    if (error) {
        std::println("Error mapping input file '{}': {}", in_filepath, error.message());
        return -1;
    }

    // create (sparse) output file with the input file size
    uint64_t source_len = source.size();
    int ret = create_sparse(out_filepath, source_len);
    if (ret != 0) {
        std::println("Error creating output file of size {} bytes", source_len);
        return -1;
    }

    const auto processor_count = std::thread::hardware_concurrency();
    ProgressBars bars;
    std::vector<std::thread> threads;
    std::promise<void> p;
    auto sf = p.get_future().share();
    std::println("Decrypting '{}' with {} threads using {}", in_filepath, processor_count, Cipher::Aes<128>::AES_TECHNOLOGY);

    //print_hex("XTS KEY", xts_key);
    //print_hex("XTS TWK", xts_tweak);

    uint32_t page_size = platform_getpagesize();
    int sectors_per_page = page_size / SECTOR_SIZE;
    //std::println("Page size is {} bytes, {} sectors per page", page_size, sectors_per_page);
    uint64_t num_sectors = source_len / SECTOR_SIZE;
    uint64_t slice_size = num_sectors / processor_count;
    if (slice_size % sectors_per_page != 0) {
        auto diff = sectors_per_page - slice_size % sectors_per_page;
        //std::println("Adjusting slice_size by {} to match page size boundary", diff);
        slice_size += diff;
        //std::println("slice_size % sectors_per_page is now {}", slice_size % sectors_per_page);
    }
    uint64_t slice_start = 0;

    // create threads
    for (size_t i = 1; i <= processor_count; ++i) {
        uint64_t slice_end = slice_start + slice_size;

        // last thread might get fewer sectors than a normal slice
        if (i == processor_count) {
            slice_end = num_sectors;
        }

        auto thread_bar_max = 101;
        auto bar_idx = bars.new_bar(i, thread_bar_max);

        threads.emplace_back([&bars, thread_bar_max, sf, i, source, out_filepath,
                              slice_start, slice_end, xts_key, xts_tweak, iv_offset](size_t bar_idx) {
            std::error_code local_error;
            mio::mmap_sink output = mio::make_mmap_sink(out_filepath, slice_start * SECTOR_SIZE, (slice_end - slice_start) * SECTOR_SIZE, local_error);
            if (local_error) {
                std::println("Thread {}: could not create mmap sink from {} for {} bytes: {}", i, slice_start * SECTOR_SIZE, (slice_end - slice_start) * SECTOR_SIZE, local_error.message());
                exit(1);
            }
            auto xts = Cipher::AES::XTS_128(xts_key, xts_tweak, SECTOR_SIZE);

            //std::println("Thread {:2d} sectors {:9d} - {:9d}", i, slice_start, slice_end);
            uint64_t input_flush_counter = 0;
            auto unused_ptr_base = &source[0];
            uint64_t one_percent = (slice_end - slice_start) * 0.01;
            uint64_t next_tick_pos = slice_start + one_percent;

            // wait for all threads to be ready
            sf.wait();

            for (uint64_t sector_index = slice_start; sector_index < slice_end; ++sector_index) {
                // release source memory every 100MiB
                if (input_flush_counter++ == (100 * 1024 * 1024 / SECTOR_SIZE)) {
                    platform_release_mmap_region((void *)unused_ptr_base, 100 * 1024 * 1024);
                    unused_ptr_base += 100 * 1024 * 1024;
                    input_flush_counter = 0;
                }

                // tick progress every 1%
                if (sector_index > next_tick_pos) {
                    bars.tick(bar_idx);
                    next_tick_pos += one_percent;
                }

                xts.crypt(Cipher::Mode::Decrypt, sector_index + iv_offset,
                          &source[SECTOR_SIZE * sector_index], 
                          &output[SECTOR_SIZE * (sector_index - slice_start)]);
            }
            bars.post_work(bar_idx, "Syncing");
            output.sync(local_error);
            if (local_error) {
                bars.set_error(bar_idx, local_error.message());
            } else {
                bars.set_complete(bar_idx, thread_bar_max);
            }
        }, bar_idx);
        slice_start += slice_size;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    // kick off threads
    p.set_value();

    // wait for everything to finish
    for (auto &t: threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    std::println("Decrypted in {}m{:02}s", elapsed_sec / 60, elapsed_sec % 60);
    std::printf("Speed %4.0f MiB/sec\n", source_len / 1024 / 1024.0 / elapsed_sec);

    return 0;
}
