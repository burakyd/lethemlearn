#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdio>

// Configuration
const int NUM_ISLANDS = 6;
const int NUM_MIGRANTS = 5;
const int MIGRATION_INTERVAL = 40000; // generations (should match island)
const int NUM_MIGRATIONS = 20; // how many migrations to perform
const std::string SIM_EXE = "main.exe"; // built from main.cpp
const std::string MIGRATION_DIR = "migration";

std::string gene_pool_file(int island) {
    return "gene_pool_" + std::to_string(island) + ".txt";
}
std::string out_file(int island) {
    return MIGRATION_DIR + "/migrants_from_" + std::to_string(island) + ".dat";
}
std::string in_file(int island) {
    return MIGRATION_DIR + "/migrants_to_" + std::to_string(island) + ".dat";
}

void launch_island_thread(int island) {
    std::string cmd = SIM_EXE
        + " --island_id " + std::to_string(island)
        + " --gene_pool_file " + gene_pool_file(island)
        + " --migration_dir " + MIGRATION_DIR
        + " --headless";
    std::system(cmd.c_str());
}

void wait_for_migrants() {
    // Wait for all islands to write their migrants_from_X.dat
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        while (!std::filesystem::exists(out_file(i))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

std::vector<std::vector<std::string>> collect_migrants() {
    // Read all migrants from each island
    std::vector<std::vector<std::string>> migrants(NUM_ISLANDS);
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        std::ifstream ifs(out_file(i));
        std::string block, line;
        while (std::getline(ifs, line)) {
            block += line + "\n";
            if (line == "END") {
                migrants[i].push_back(block);
                block.clear();
            }
        }
        ifs.close();
        std::remove(out_file(i).c_str());
    }
    return migrants;
}

void redistribute_migrants(const std::vector<std::vector<std::string>>& migrants) {
    // Round robin: send migrants[i] to (i+1)%N
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        int dest = (i + 1) % NUM_ISLANDS;
        std::ofstream ofs(in_file(dest), std::ios::app);
        for (const auto& block : migrants[i]) {
            ofs << block;
        }
        ofs.close();
    }
}

void print_fitness_logs() {
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        std::string log_file = MIGRATION_DIR + "/fitness_log_island_" + std::to_string(i) + ".txt";
        std::ifstream ifs(log_file);
        std::string line, last_line;
        while (std::getline(ifs, line)) {
            if (!line.empty()) last_line = line;
        }
        if (!last_line.empty()) {
            std::cout << "[Island " << i << "] " << last_line << std::endl;
        } else {
            std::cout << "[Island " << i << "] No fitness log found." << std::endl;
        }
    }
}

void clean_migration_dir() {
    for (const auto& entry : std::filesystem::directory_iterator(MIGRATION_DIR)) {
        const std::string fname = entry.path().filename().string();
        if (fname.find("migrants_from_") == 0 || fname.find("migrants_to_") == 0 || fname.find("stop_island_") == 0 || fname.find("fitness_log_island_") == 0) {
            std::filesystem::remove(entry.path());
        }
    }
    std::cout << "[Master] Cleaned migration directory." << std::endl;
}

int main() {
    std::filesystem::create_directory(MIGRATION_DIR);
    clean_migration_dir();
    // Launch islands in parallel
    std::vector<std::thread> island_threads;
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        island_threads.emplace_back(launch_island_thread, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Add delay between launches
    }
    for (int mig = 0; mig < NUM_MIGRATIONS; ++mig) {
        std::cout << "[Master] Migration round " << mig << std::endl;
        wait_for_migrants();
        auto migrants = collect_migrants();
        redistribute_migrants(migrants);
        std::cout << "[Master] Migration " << mig << " complete." << std::endl;
        print_fitness_logs();
    }
    // Signal all islands to stop
    for (int i = 0; i < NUM_ISLANDS; ++i) {
        std::ofstream ofs(MIGRATION_DIR + "/stop_island_" + std::to_string(i));
        ofs << "stop";
        ofs.close();
    }
    std::cout << "[Master] Stop signals sent to all islands." << std::endl;
    std::cout << "[Master] Done. You may want to kill the islands manually if they run forever." << std::endl;
    // Join all threads before exiting
    for (auto& t : island_threads) {
        if (t.joinable()) t.join();
    }
    return 0;
} 