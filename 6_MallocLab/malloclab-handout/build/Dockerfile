# Docker file for 32-bit build with makefile

FROM ubuntu:latest

# Enable support for i386 architecture
RUN dpkg --add-architecture i386

# Update the package list
RUN apt-get update

# Install 32-bit libraries
RUN apt-get install -y libc6:i386 libncurses5:i386 build-essential gcc-multilib make

# Set the working directory in the container
WORKDIR /app
