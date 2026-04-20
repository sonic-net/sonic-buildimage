import os
import shutil
import glob
import jinja2

def render(dry_run: bool):
 
    in_yang_templates = "./yang-templates"
    in_yang_models = "./yang-models"

    output_base = "."
    if "RULEDIR" in os.environ:
        output_base = os.environ["RULEDIR"]

    out_clvyang_models = f"{output_base}cvlyang-models"
    out_yang_models = f"{output_base}yang-models"

    if not os.path.exists(out_yang_models):
        os.makedirs(out_yang_models)
    
    if not os.path.exists(out_clvyang_models):
        os.makedirs(out_clvyang_models)

    # copy non-template yang model to internal yang model directory
    for fname in glob.glob(f"{in_yang_models}/*.yang"):
        bfname = os.path.basename(fname)
        shutil.copyfile(f"{in_yang_models}/{bfname}", f"{out_clvyang_models}/{bfname}")
    
    # templated yang models
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(in_yang_templates), trim_blocks=True)
    for fname in glob.glob(f"{in_yang_templates}/*.yang.j2"):
        bfname = os.path.basename(fname)
        template = env.get_template(bfname)
        yang_model = template.render(yang_model_type="py")
        cvlyang_model = template.render(yang_model_type="cvl")
        with open("{}/{}".format(out_yang_models, bfname.strip(".j2")), 'w') as f:
            f.write(yang_model)
        with open("{}/{}".format(out_clvyang_models, bfname.strip(".j2")), 'w') as f:
            f.write(cvlyang_model)


if __name__ == "__main__":
    render(dry_run = False)
