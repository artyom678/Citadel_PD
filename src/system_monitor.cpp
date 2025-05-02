#include "system_monitor.hpp"
#include "config.hpp"
#include "metrics.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>


SystemMonitor::SystemMonitor(const Config& config)
    : period(config.get_period())
    , metrics_config(config.get_metrics())
    , outputs(config.get_outputs())
{
	config.setup_logging(log_file);
}

SystemMonitor::~SystemMonitor() {

	if (log_file.is_open()) {
		
		log_file.close(); //not really necessary, as file closes automatically in a ~std::ifstream() 
	}
}

void SystemMonitor::run() {
	
	std::cout << "Starting system monitor with period " << period << "s" << std::endl;

	while(true) {
		auto metrics = collect_metrics();
		output_metrics(metrics);
		std::this_thread::sleep_for(std::chrono::seconds(period));
	}

}


std::vector<CpuStats> SystemMonitor::get_cpu_times() const {

	std::ifstream proc_stat("/proc/stat");
	if (!proc_stat) {
		throw std::runtime_error("Failed to open the /proc/stat file to get cpu statistics");
	}
	std::vector<CpuStats> times;
	std::string line;

	while(std::getline(proc_stat, line)) {
		
		if (line.find("cpu") == 0) {

			if (line[3] == ' ') continue;

			std::istringstream iss(line);
			std::string cpu; // this is just to skip cpuN
			
			CpuStats stats;
			uint64_t user, nice
				, system, idle
				, iowait, irq
				, softirq, steal;

			iss >> cpu >> stats.user 
				>> stats.nice >> stats.system 
				>> stats.idle >> stats.iowait 
				>> stats.irq >> stats.softirq 
				>> stats.steal;

			times.push_back(stats);
		}
		else {
			break;
		}

	}

	return times;
}


std::vector<std::unique_ptr<Metric>> SystemMonitor::collect_cpu_metrics(const json& metric) const {

	std::vector<int> ids = metric["ids"].get<std::vector<int>>();
	static auto prev_times = get_cpu_times();

	auto curr_times = get_cpu_times();
	std::vector<std::unique_ptr<Metric>> cpu_metrics;

	for(int id : ids) {
		
		if (id >= 0 && id < curr_times.size()) {
			
			auto total_diff = curr_times[id].total() - prev_times[id].total();
			auto active_diff = curr_times[id].active() - prev_times[id].active();

			double load = (total_diff > 0) ? (static_cast<double>(active_diff) / total_diff) * 100.0 : 0.0;
			cpu_metrics.emplace_back(new CpuLoad(id, load));
		}
		else {
			throw std::invalid_argument("cpu-id '" + std::to_string(id) + "' in configuration file is invalid"); //
		}
	}

	prev_times = std::move(curr_times);

	return cpu_metrics;
}


void SystemMonitor::read_mem_info(double& mem_total, double& mem_free, double& mem_available) const {

	std::ifstream meminfo("/proc/meminfo");
	if (!meminfo) {
		throw std::runtime_error("Failed to open /proc/meminfo");
	}

	std::string line;
	std::uint64_t total_kb = 0, available_kb = 0, free_kb = 0;

	while(std::getline(meminfo, line)) {

		std::istringstream iss(line);
		std::string key;
		std::uint64_t value;
		std::string unit;

		iss >> key >> value >> unit;

		if (key == "MemTotal:") {
			total_kb = value;
		}
		else if (key == "MemFree:") {
			free_kb = value;
		}
		else if (key == "MemAvailable:") {
			available_kb = value;
		}
		else {
			break;
		}
	} 	

	if (total_kb == 0 || available_kb == 0) {
        throw std::runtime_error("Failed to read required fields from /proc/meminfo");
	}

	mem_total = total_kb / (1024.0 * 1024);
	mem_free = free_kb / (1024.0 * 1024);
	mem_available = available_kb / (1024.0 * 1024);	
}



std::vector<std::unique_ptr<Metric>> SystemMonitor::collect_memory_metrics(const json& metric) const {

	auto specs = metric["spec"].get<std::vector<std::string>>(); // extracting ["used", "free"]
	double mem_total = 0.0, mem_free = 0.0, mem_available = 0.0;

	read_mem_info(mem_total, mem_free, mem_available);

	std::vector<std::unique_ptr<Metric>> mem_metrics;

	for(const auto& spec : specs) {
		if (spec == "used") {
			mem_metrics.emplace_back(new MemoryMetric(spec, mem_total - mem_available)); // to do: add a custom allocator (stack or linear would be enough)
		}
		else if (spec == "free") {
			mem_metrics.emplace_back(new MemoryMetric(spec, mem_free)); // new...
		}
	}

	return mem_metrics;
}



std::vector<std::unique_ptr<Metric>> SystemMonitor::collect_metrics() const {

	std::vector<std::unique_ptr<Metric>> collected_metrics;

	for (json metric : metrics_config) {
		
		if (metric["type"] == "cpu") {
			
			auto cpu_metrics = collect_cpu_metrics(metric);
			
			collected_metrics.insert(collected_metrics.end()
				, std::make_move_iterator(cpu_metrics.begin())
				, std::make_move_iterator(cpu_metrics.end())
				);
		}
		else if (metric["type"] == "memory") {
			
			auto memory_metrics = collect_memory_metrics(metric);

			collected_metrics.insert(collected_metrics.end()
				, std::make_move_iterator(memory_metrics.begin())
				, std::make_move_iterator(memory_metrics.end())
				);
		}
	}

	return collected_metrics;
}



void SystemMonitor::output_metrics(const std::vector<std::unique_ptr<Metric>>& metrics) {
    
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();

    std::vector<std::string> console_strings;
    console_strings.reserve(metrics.size());
    for (const auto& metric : metrics) {
        console_strings.push_back(metric->to_string());
    }
    std::string console_output = "Metrics at " + timestamp + ": " + join(console_strings, "; ");

    for (const auto& output : outputs) {
        if (output["type"] == "console") {

            std::cout << console_output << std::endl;
        
        } else if (output["type"] == "log") {

        	if (!log_file) {
        		throw std::runtime_error("Cannot write to the log file, it's closed");
        	}

            json log_entry;
            log_entry["timestamp"] = timestamp;
            log_entry["metrics"] = json::array();
            for (const auto& metric : metrics) {
                log_entry["metrics"].push_back(metric->to_json());
            }
            log_file << log_entry.dump(2) << std::endl;
            log_file.flush();
        }
    }
}




std::string SystemMonitor::join(const std::vector<std::string>& vec, const std::string& delim) const {
    
    if (vec.empty()) return "";
    std::string result = vec[0];
    for (size_t i = 1; i < vec.size(); ++i) {
        result += delim + vec[i];
    }
    return result;
}