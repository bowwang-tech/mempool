import argparse

def generate_header_file(array_length):
    filename = "data.h"

    with open(filename, "w") as file:
        # Comment about the file's purpose
        file.write("// This data file is only for testing copy data from L2 to L1 with cores\n\n")
        
        # Define the ARRAY_SIZE macro
        file.write("#define ARRAY_SIZE ({})\n\n".format(array_length))
        # Write the array
        file.write("uint32_t l2_data_flat[{}] = {{\n".format(array_length))

        for i in range(array_length):
            file.write("\t{}".format(i))
            if i < array_length - 1:
                file.write(", ")  # Add a comma except for the last element
            if (i + 1) % 8 == 0 or i == array_length - 1:
                file.write("\n")  # New line every 8 numbers or at the end
        
        file.write("};\n")
        file.write("// int32_t(*l2_data) = (int32_t(*))l2_data_flat;")
    return filename

# Example usage for testing
# generated_file = generate_header_file(1024)

def main():
    parser = argparse.ArgumentParser(description='Generate a C header file with a specified array length.')
    parser.add_argument('length', type=int, help='Length of the array to be generated.')
    
    args = parser.parse_args()
    array_length = args.length

    generated_file = generate_header_file(array_length)
    print(f"Generated file {generated_file} with array length {array_length}")

if __name__ == "__main__":
    main()