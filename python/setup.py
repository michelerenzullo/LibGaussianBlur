from setuptools import setup, Extension, find_packages
import os

# Define a dummy extension so that the wheel won't have 'none-any.whl' suffix
# but 'macosx_15_0_arm64.whl' etc...
#
# Dynamically write a source file (otherwise clang will complain about no inputs)
with open("dummy.c", "w") as f:
    f.write("""
            void init_dummy(void) {}
            """)
dummy_extension = Extension("gaussianblur._dummy", sources=['dummy.c'])

try:
    setup(
        name='gaussianblur',
        version='1.1.2',
        packages=find_packages(),
        package_data={'gaussianblur': [
            'gaussianblur.cpython*.so',
            'libGaussianblur.*'
            ]},
        include_package_data=True,
        ext_modules=[dummy_extension],
        zip_safe=False
    )
finally:
    # Clean up the dummy.c file after the build
    if os.path.exists("dummy.c"):
        os.remove("dummy.c")
