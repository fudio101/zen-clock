import os
import sys
import subprocess
from pathlib import Path

def ensure_clang_format():
    try:
        import clang_format
    except ImportError:
        print("clang-format Python package is not installed. Installing it now...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "clang-format"])
        print("Successfully installed clang-format.")

def get_source_files(root_dir, targets=None):
    files_to_format = []
    
    if not targets:
        # Default directories
        targets = ["src", "include", "components"]
        # Convert relative defaults to absolute paths based on root_dir
        targets = [str(root_dir / d) for d in targets]
        
    for target in targets:
        target_path = Path(target)
        if not target_path.exists():
            print(f"Warning: Target '{target}' does not exist.")
            continue
            
        if target_path.is_file():
            # If it's a file, add it directly if it has a valid extension
            if target_path.suffix in [".c", ".h", ".cpp", ".hpp"]:
                files_to_format.append(str(target_path.absolute()))
        else:
            # If it's a directory, scan for valid extensions
            for ext in ["*.c", "*.h", "*.cpp", "*.hpp"]:
                for file_path in target_path.rglob(ext):
                    # Skip managed components and build directories
                    if "managed_components" in str(file_path) or ".pio" in str(file_path) or "build" in str(file_path):
                        continue
                    files_to_format.append(str(file_path.absolute()))
                
    # Remove duplicates just in case
    return list(set(files_to_format))

def main():
    ensure_clang_format()
    
    project_root = Path(__file__).parent.parent
    
    verbose = "-v" in sys.argv
    # Get targets (everything else that is not script name or -v)
    targets = [arg for arg in sys.argv[1:] if arg != "-v"]
    
    files = get_source_files(project_root, targets)
    
    if not files:
        print("No source files found to format.")
        return

    verbose = "-v" in sys.argv
    
    if verbose:
        print(f"Formatting {len(files)} files:")
        for f in files:
            print(f"  - {Path(f).relative_to(project_root)}")
    else:
        print(f"Formatting {len(files)} files...")
    
    # Get the cross-platform executable path from the package
    import clang_format
    clang_exe = clang_format.get_executable("clang-format")
    
    cmd = [clang_exe, "-i"] + files
    
    try:
        subprocess.check_call(cmd, cwd=project_root)
        print("Formatting complete!")
    except subprocess.CalledProcessError as e:
        print(f"Error formatting files: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
