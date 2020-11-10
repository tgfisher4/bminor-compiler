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

echo "Printer tests..."
cd printer_tests
./run_all_tests.sh
echo "=========================================="
cd ..

echo "Resolver tests..."
cd resolver_tests
./run_all_tests.sh
echo "=========================================="
cd ..

echo "Typechecker tests..."
cd typechecker_tests
./run_all_tests.sh
echo "=========================================="
cd ..
