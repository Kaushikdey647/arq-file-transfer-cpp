import setuptools

setuptools.setup(
    name='arq',
    version='0.0.1',
    author='Kaushik Dey',
    author_email='20cs01043@iitbbs.ac.in',
    description='A Python interface for the arq C++ class',
    packages=setuptools.find_packages(),
    ext_modules=[
        setuptools.Extension(
            'arq',
            sources=['pylib.cpp'],
            extra_compile_args=['-std=c++11'],
            libraries=['arq']
        )
    ],
)