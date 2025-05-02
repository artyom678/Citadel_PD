#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>
#include <string>
#include "config.hpp"
#include "metrics.hpp"

using json = nlohmann::json;

struct SystemMonitor {

	SystemMonitor(const Config& config);

	~SystemMonitor();

	void run();

private:

	std::vector<CpuStats> get_cpu_times() const;

	std::vector<std::unique_ptr<Metric>> collect_cpu_metrics(const json& metric) const;

	std::vector<std::unique_ptr<Metric>> collect_memory_metrics(const json& metric) const;

	std::vector<std::unique_ptr<Metric>> collect_metrics() const;

	void output_metrics(const std::vector<std::unique_ptr<Metric>>& metrics);

	//void console_output(const std::vector<std::unique_ptr<Metric>>& metrics) const;

	//void json_log_output(const std::vector<std::unique_ptr<Metric>>& metrics);

	void read_mem_info(double& mem_total, double& mem_free, double& mem_availible) const;

	std::string join(const std::vector<std::string>& vec, const std::string& delim) const;

private:
	
	int period; // how often we should check the metrics
	std::vector<json> metrics_config; // the metrics (cpu-load, free memory, etc.)
	std::vector<json> outputs; // where we should put the output
	std::ofstream log_file;
};