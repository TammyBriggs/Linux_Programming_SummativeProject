from setuptools import setup, Extension

# Define the C extension module
vibration_module = Extension(
    'vibration', 
    sources=['vibration.c'],
    extra_compile_args=['-O3', '-Wall'] # Optimize for speed, show warnings
)

setup(
    name='vibration_analytics',
    version='1.0',
    description='C Extension for Real-time Industrial Vibration Analysis',
    ext_modules=[vibration_module]
)
