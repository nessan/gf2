// Check our char-poly computation vs. some pre-canned ones done by other means.
// **Note:**  This should be run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <fstream>

#include <utilities/stopwatch.h>
#include <utilities/stream.h>
#include <gf2/namespace.h>

int
main() {
    using word_type = usize;
    using matrix_type = BitMat<word_type>;
    using coeff_type = BitVec<word_type>;
    using poly_type = BitPoly<word_type>;

    // A timer
    utilities::stopwatch sw;

    // Get the name of the data file and open it ...
    std::string   data_file_name;
    std::ifstream data_file;
    while (true) {
        std::cout << "Data file name (x to exit ...): ";
        std::cin >> data_file_name;
        if (data_file_name == "x" || data_file_name == "X") exit(0);
        data_file.open(data_file_name);
        if (data_file.is_open()) break;
        std::print("Failed to open '{}'. Please try again ...\n", data_file_name);
    }

    // The pre-canned results are in a file with the format:
    // - A bit-matrix (row by row with the rows delimited by commas, white space, semi-colons etc.)
    // - The coefficients of the corresponding characteristic polynomial as a bit-vector.
    // There can be many of these pairs of lines in the file and we iterate through them all.
    // Our `utilities::read_line` ignores blank lines and comment lines (which start with a "#").
    std::string matrix_string;
    std::string coeffs_string;
    size_t      n_test = 0;
    while (utilities::read_line(data_file, matrix_string) != 0 && utilities::read_line(data_file, coeffs_string) != 0) {

        // Get the bit-matrix (from a single potentially very long line!)
        auto m = matrix_type::from_string(matrix_string);
        if (!m) {
            std::println("Failed to parse a bit-matrix from file: '{}'", data_file_name);
            exit(1);
        }

        // Get the corresponding pre-canned characteristic polynomial coefficients.
        auto coeff = coeff_type::from_string(coeffs_string);
        if (!coeff) {
            std::println("Failed to parse a characteristic polynomial from file: '{}'", data_file_name);
            exit(2);
        }
        poly_type c{*coeff};

        // Progress meter
        ++n_test;
        std::print("Test {} of {}: Matrix is {} x {} ... ", n_test, n_test, m->rows(), m->cols());

        // Compute our own version of the characteristic polynomial.
        sw.click();
        auto p = m->characteristic_polynomial();
        sw.click();
        auto lap = sw.lap();
        std::println("done in {:.2f}s.", lap);

        // Check our version matches the pre-canned version.
        if (p != c) {
            std::println("TEST {} FAILED! Matrix:\n {}", n_test, *m);
            std::println("Computed characteristic:   {}", p);
            std::println("Pre-canned characteristic: {}", c);
            exit(1);
        }
    }
    data_file.close();

    // Only get here on success
    std::println("\n Congratulations: All {} tests passed!", n_test);
    return 0;
}
