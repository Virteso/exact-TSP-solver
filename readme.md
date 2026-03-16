# Setup
## Quick
```
git clone https://github.com/Virteso/exact-TSP-solver.git
bash setup.sh venv_name
```

## Step by step
Pull with submodules:  
```
git clone --recurse-submodules https://github.com/Virteso/exact-TSP-solver.git
```

Make a python virtual environment:  
```
python -m venv <venv_name>
```

Activate the virtual environment and install dependencies:
```
source <venv_name>/bin/activate
pip install -r python/requirements.txt
```