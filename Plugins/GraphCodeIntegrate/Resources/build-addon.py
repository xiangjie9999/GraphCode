"""Package the Blender companion without generated files or local preferences."""
import argparse
from pathlib import Path
import zipfile

parser = argparse.ArgumentParser(description="Build the GraphCode Integrate Blender add-on ZIP")
parser.add_argument("output", type=Path)
args = parser.parse_args()
source = Path(__file__).parent/"blender"/"graphcode_integrate"
args.output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(args.output,"w",zipfile.ZIP_DEFLATED) as archive:
    for filename in sorted(source.glob("*.py")):
        archive.write(filename,"graphcode_integrate/"+filename.name)
print(args.output.resolve())
