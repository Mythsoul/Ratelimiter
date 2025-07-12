# Use Ubuntu as base image for C++ compilation
FROM ubuntu:22.04

# Install build essentials and necessary packages
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY index.cpp .

# Compile the C++ application
RUN g++ -o server index.cpp -std=c++11

# Expose the port that the app runs on
EXPOSE 10000

# Command to run the application
CMD ["./server"]
