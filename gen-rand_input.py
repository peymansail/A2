import random

# Function to generate a list of random numbers and write to a file in C format
def generate_random_numbers_c_format(file_name, num_numbers, min_value, max_value):
    # Generate a list of random numbers
    random_numbers = [random.randint(min_value, max_value) for _ in range(num_numbers)]
    
    # Format the list in C array style
    c_format_numbers = "{" + ", ".join(map(str, random_numbers)) + "};"
    
    # Write the formatted numbers to the output file
    with open(file_name, 'w') as file:
        file.write(c_format_numbers)
    
    print(f"{num_numbers} random numbers have been written to {file_name} in an array format.")

# Parameters
output_file = "input.txt"  # Output file name
number_count = 50000  # Number of random numbers to generate
min_val = 1  # Minimum value of random numbers
max_val = 1000  # Maximum value of random numbers

# Call the function
generate_random_numbers_c_format(output_file, number_count, min_val, max_val)
