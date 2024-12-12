# !/bin/bash

# This script is used to run the rocm_bandwidth_test tool
# Usage: ./run_rocm_bandwidth_tool.sh <path to rocm_bandwidth_test binary>
# The script prompts the user to enter the list of gpus to run the test on, and the list of message sizes to run the test on
# Based on the list of gpus entered by the user, the script finds the numa nodes associated with each of the gpus, and runs the tool with sources as the numa nodes and destinations as the gpus (h2d) and then with sources as the gpus and destinations as the numa nodes (d2h) with the user entered message sizes
# If no message sizes are entered by the user, the script defaults to 64MB
# Link to rocm-bandwidth-test tool: https://github.com/RadeonOpenCompute/rocm_bandwidth_test


# check if the user entered the path to the rocm_bandwidth_test binary, otherwise default to current directory
if [ -z "$1" ]; then
    echo "No path to rocm_bandwidth_test binary entered. Defaulting to current directory"
    path=$(pwd)

else 
    path=$1
fi

# take a comma separated list of gpus (0-15) from the user
function read_gpus {
    echo "Enter the comma separated list of gpus (0-15) to run the test on"
    read gpus
    echo "You entered: $gpus"
}

# take a list of message sizes from the user, default to 64MB if no input is provided
function read_message_sizes {
    echo "Enter the comma separated list of message sizes to run the test on (in MB, e.g 64,128,256) (default: 64)"
    read message_sizes
    echo "You entered: $message_sizes"
    # default env variable to 64MB if no input is provided
    if [ -z "$message_sizes" ]; then
        echo "No message sizes entered. Defaulting to 64MB"
        message_sizes="64"
    fi
    # validate the inputted message sizes
    # check if the user entered a comma separated list of numbers
    if [[ ! $message_sizes =~ ^[0-9]+(,[0-9]+)*$ ]]; then
        echo "Invalid input. Please enter a comma separated list of numbers"
        exit 1
    fi
    # check if the user entered a valid message size
    for message_size in $(echo $message_sizes | sed "s/,/ /g")
    do
        if [ $message_size -lt 0 ] || [ $message_size -gt 1000000000 ]; then
            echo "Invalid input. Please enter a comma separated list of numbers between 0 and 1000000000"
            exit 1
        fi
    done
}

# validate the user input
function validate_gpus {
    # check if the user entered a comma separated list of numbers
    if [[ ! $gpus =~ ^[0-9]+(,[0-9]+)*$ ]]; then
        echo "Invalid input. Please enter a comma separated list of numbers"
        exit 1
    fi

    # check if the user entered a valid gpu number
    for gpu in $(echo $gpus | sed "s/,/ /g")
    do
        if [ $gpu -lt 0 ] || [ $gpu -gt 15 ]; then
            echo "Invalid input. Please enter a comma separated list of numbers between 0 and 15"
            exit 1
        fi
    done
}

# Get the NUMA nodes associated with each of the gpus entered by the user
function get_numa_nodes {
    for gpu in $(echo $gpus | sed "s/,/ /g")
    do
        echo "Finding numa node associated with GPU $gpu"
        # get the numa node associated with the gpu
        numa_node=$(cat /sys/class/drm/card$gpu/device/numa_node)
        # print the numa node associated with the gpu
        echo "GPU $gpu is associated with NUMA node $numa_node"
        numa_nodes+=($numa_node)
    done
    # remove duplicates from list of numa nodes, convert into comma separated list
    numa_nodes=$(echo "${numa_nodes[@]}" | tr ' ' '\n' | sort -u | tr '\n' ',' | sed 's/,$//')
    echo "NUMA nodes associated with the GPUs entered by the user: ${numa_nodes[@]}"
}

# Run the tool with sources as the numa_nodes and destinations as the gpus (h2d)
function run_h2d {
    # command to be executed
    echo "$path/rocm-bandwidth-test -d $gpus -s $numa_nodes -m $message_sizes"
    $path/rocm-bandwidth-test -d $gpus -s $numa_nodes -m $message_sizes
}

# Run the tool with sources as the gpus and destinations as the cpus (d2h)
function run_d2h {
    # command to be executed
    echo "$path/rocm-bandwidth-test -d $numa_nodes -s $gpus -m $message_sizes"
    $path/rocm-bandwidth-test -d $numa_nodes -s $gpus -m $message_sizes
}


read_gpus
read_message_sizes
validate_gpus
get_numa_nodes
run_h2d
run_d2h
