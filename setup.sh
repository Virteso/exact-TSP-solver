#!/bin/bash
set -e

# Try to pull all submodules
git submodule update --init --recursive

# Use first positional argument as venv name, or default to 'pyvenv'
VENV_NAME="${1:-pyvenv}"

echo "Creating virtual environment: $VENV_NAME"
python3 -m venv "$VENV_NAME"
source "$VENV_NAME/bin/activate"
pip install --upgrade pip
pip install -r python/requirements.txt

echo "Setup complete. Virtual environment '$VENV_NAME' is ready."