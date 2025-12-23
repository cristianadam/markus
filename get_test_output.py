import sys

# Allow importing commonmark test helpers
sys.path.insert(0, 'commonmark-spec/test')
from spec_tests import get_tests

def main():
    # Parse test numbers from command-line args
    # Example: python run_specific_tests.py 256 282 283
    if len(sys.argv) > 1:
        try:
            selected = {int(arg) for arg in sys.argv[1:]}
        except ValueError:
            print("All test numbers must be integers")
            sys.exit(1)
    else:
        selected = None  # no filtering

    tests = get_tests('commonmark-spec/spec.txt')

    for t in tests:
        if selected is None or t['example'] in selected:
            print(f"Test {t['example']}:")
            print("Markdown:", repr(t['markdown']))
            print("Expected:", repr(t['html']))
            print()

if __name__ == "__main__":
    main()
