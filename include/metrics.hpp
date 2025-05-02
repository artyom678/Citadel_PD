#pragma once

#include <cstdint>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using json = nlohmann::json;

struct Metric {

	virtual std::string to_string() const = 0;

	virtual json to_json() const = 0;

	virtual ~Metric() = default;

	std::string double_to_string(double value, int precision) const {

		std::stringstream ss;
		ss << std::fixed << std::setprecision(precision) << value;

		return ss.str();
	}

};


struct CpuLoad : Metric {

	CpuLoad(int cpu_id, double load) noexcept : cpu_id(cpu_id), load(load) {}
	
	std::string to_string() const override { // cringe
        
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "CPU%d: %.2f%%", cpu_id, load);
        return buffer;
    }

    json to_json() const override {
        json j;
        j["type"] = "cpu";
        j["id"] = cpu_id;
        j["load"] = double_to_string(load, 2);
        return j;
    }


	int cpu_id;
	double load;
};


struct MemoryMetric : Metric {

	MemoryMetric(const std::string& spec, double value) : spec(spec), value(value) {}

    std::string to_string() const override {
        
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "Memory %s: %.2f GB", spec.c_str(), value);
        return buffer;
    }

    json to_json() const override {
        
        json j;
        j["type"] = "memory";
        j["spec"] = spec;
        j["value"] = double_to_string(value, 2);
        return j;
    }


	std::string spec; // 
	double value;
};


//helper-class for calculating the load on a cpu
struct CpuStats {

	std::uint64_t user
		, nice
		, system
		, idle
		, iowait
		, irq
		, softirq
		, steal;

	inline std::uint64_t total() const {
		return user +
			+ nice +
			+ system +
			+ idle +
			+ iowait +
			+ irq +
			+ softirq +
			+ steal;
	}

	inline std::uint64_t active() const {
		return total() - idle - iowait;
	}

};
