import os
import sys


def process_files(input_dir):
    output_dir = "test-app\/malware_dataset"

    # Create output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"Created directory: {output_dir}")

    # The wrapper template
    wrapper_start = (
        'setJitCompilerOption("baseline.warmup.trigger", 0);\n'
        'setJitCompilerOption("ion.warmup.trigger", 0);\n'
        'function target(){\n'
    )
    wrapper_end = '\n}\ntarget()'

    processed_count = 0

    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.endswith(".js"):
                source_path = os.path.join(root, file)

                try:
                    # Read the original code
                    with open(source_path, 'r', encoding='utf-8', errors='ignore') as f:
                        original_code = f.read()

                    # Construct the new content
                    new_content = f"{wrapper_start}{
                        original_code}{wrapper_end}"

                    # Determine unique output filename (handle collisions)
                    output_filename = file
                    output_path = os.path.join(output_dir, output_filename)

                    counter = 1
                    base_name, ext = os.path.splitext(output_filename)
                    while os.path.exists(output_path):
                        output_filename = f"{base_name}_{counter}{ext}"
                        output_path = os.path.join(output_dir, output_filename)
                        counter += 1

                    # Write the modified code
                    with open(output_path, 'w', encoding='utf-8') as f:
                        f.write(new_content)

                    processed_count += 1
                    print(f"Processed: {file} -> {output_filename}")

                except Exception as e:
                    print(f"Error processing {source_path}: {e}")

    print(f"\nDone. {processed_count} files saved to '{output_dir}'.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python script.py <directory_path>")
    else:
        target_directory = sys.argv[1]
        if os.path.isdir(target_directory):
            process_files(target_directory)
        else:
            print("Error: The provided argument is not a valid directory.")
