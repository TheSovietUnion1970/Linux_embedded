#!/bin/bash
(make -j$(nproc) clean) || { echo "Failed to clean $dir"; exit 1; }
echo "All drivers cleaned successfully!"
