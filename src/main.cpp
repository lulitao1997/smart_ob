// #include <fstream>
// #include <iostream>

// #include "pnl_calculator.h"
// #include "reader.h"

// // takes two command line arguments:
// // The path to an input file of trades executed throughout the day. See below
// // for the input format. A string, either fifo or lifo, indicating which
// // accounting scheme to use.
int main(int argc, char *argv[]) {
}
// int main(int argc, char *argv[]) {
//     if (argc != 3 || (std::string(argv[2]) != "fifo" && std::string(argv[2]) != "lifo")) {
//         std::cerr << "Usage: " << argv[0] << " <input_file> <fifo|lifo>" << std::endl;
//         return 1;
//     }

//     std::ifstream input_file(argv[1]);
//     if (!input_file.is_open()) {
//         std::cerr << "Error opening file: " << argv[1] << std::endl;
//         return 1;
//     }

//     bool use_lifo = (std::string(argv[2]) == "lifo");

//     auto trades = read_from_csv(input_file);
//     auto pnl_records = calculate_pnl(trades, use_lifo);

//     std::cout << "TIMESTAMP,SYMBOL,PNL" << std::endl;
//     for (const auto &record : pnl_records) {
//         std::cout << pnl_to_string(record) << std::endl;
//     }

//     return 0;
// }