"""Private quote search paths inside the original source owner repository."""
def private_includes(paths):
    repository = native.repository_name().lstrip("@")
    prefix = "external/" + repository + "/" if repository else ""
    return ["-iquote" + prefix + path for path in paths]
