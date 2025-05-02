#pragma once

#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>

using json = nlohmann::json;


struct Config {

    Config(int argc, char* argv[]) {

        parse_command_line(argc, argv);
        load_config();
        validate_config();
    }

    int get_period() const {
        return period;
    }

    const std::vector<json>& get_metrics() const {
        return metrics;
    }

    const std::vector<json>& get_outputs() const {
        return outputs;
    }

    void setup_logging(std::ofstream& log_file) const {

        for (const auto& output : outputs) {
            
            if (output["type"] == "log") {
                
                if (!output.contains("path") || !output["path"].is_string()) {
                    
                    throw std::runtime_error("Output 'log' requires a 'path' field with a string value");
                }
                
                std::string path = output["path"].get<std::string>();
                log_file.open(path, std::ios::app);
                
                if (!log_file.is_open()) {
                    
                    throw std::runtime_error("Failed to open log file: " + path);
                }
            }
        }
    }

private:

    void parse_command_line(int argc, char* argv[]) {

        config_path = "config.json"; // Значение по умолчанию
        if (argc != 2) {
        	throw std::runtime_error("Expected exactly 1 argument - the path to the JSON-file with configuration");
        }

        config_path = argv[1];

        if (config_path.empty()) {
        	throw std::runtime_error("a path cannot be empty");
        }
    }

    void load_config() {

        std::ifstream file(config_path);
        if (!file.is_open()) {    
            throw std::runtime_error("Failed to open config file: " + config_path);
        }

        try {

            file >> config_data;

        } catch (const json::exception& e) {
            
            throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
        }
    }

    void validate_config() {

        validate_period();
        validate_metrics();
        validate_outputs();
    }

    void validate_period() {

    	if (!config_data.contains("settings") || !config_data["settings"].is_object() ||
            !config_data["settings"].contains("period") || !config_data["settings"]["period"].is_number_integer()) 
    	{    
            throw std::runtime_error("Config must contain 'settings.period' as an integer");
        }

        period = config_data["settings"]["period"].get<int>();
        if (period <= 0) {
            throw std::runtime_error("Period must be a positive integer");
        }
    }

    void validate_metrics() {

    	if (!config_data.contains("metrics") || !config_data["metrics"].is_array()) {
            
            throw std::runtime_error("Config must contain 'metrics' as an array");
        }

        metrics = config_data["metrics"].get<std::vector<json>>();
        
        for (const auto& metric : metrics) {
            
            if (!metric.contains("type") || !metric["type"].is_string()) {
                
                throw std::runtime_error("Each metric must have a 'type' field as a string");
            }

            std::string type = metric["type"].get<std::string>();
            if (type == "cpu") {
                
                if (!metric.contains("ids") || !metric["ids"].is_array()) {
                    throw std::runtime_error("CPU metric must have an 'ids' array");
                }

            } else if (type == "memory") {
                
                if (!metric.contains("spec") || !metric["spec"].is_array()) {
                    throw std::runtime_error("Memory metric must have a 'spec' array");
                }

            } else {
                
                throw std::runtime_error("Unknown metric type: " + type);
            }
        }
    }

    void validate_outputs() {

    	if (!config_data.contains("outputs") || !config_data["outputs"].is_array()) {
            
            throw std::runtime_error("Config must contain'outputs' as an array");
        }

        outputs = config_data["outputs"].get<std::vector<json>>();
        
        for (const auto& output : outputs) {
            
            if (!output.contains("type") || !output["type"].is_string()) {
                
                throw std::runtime_error("Each output must have a 'type' field as a string");
            }

            std::string type = output["type"].get<std::string>();
            if (type != "console" && type != "log") {
                
                throw std::runtime_error("Unknown output type: " + type);
            }

            if (type == "log" && (!output.contains("path") || !output["path"].is_string())) {
                
                throw std::runtime_error("Log output must have a 'path' field as a string");
            }
        }
    }


private:

	std::string config_path;
    json config_data;
    int period;
    std::vector<json> metrics;
    std::vector<json> outputs;
};