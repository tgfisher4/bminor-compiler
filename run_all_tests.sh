#! /usr/bin/env bash

echo "Scanner tests..."
cd scanner_tests
./run_all_tests.sh
echo "=========================================="
cd ..

echo ""

echo "Parser tests..."
cd parser_tests
./run_all_tests.sh
echo "=========================================="
cd ..
