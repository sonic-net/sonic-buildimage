. as $lock
| $selection[0] as $view
| ($view.packages | to_entries | map(
    select(.value.name as $name | $view.root_packages | index($name))
    | {key: (.key | split("=")[0]), value: .value.version}
  ) | from_entries) as $roots
| {
    version: $lock.version,
    sources: $lock.sources,
    dependency_sets: {($subset): {sets: {($view.architecture): $roots}}},
    packages: ($view.packages | with_entries(.value = $lock.packages[.key]))
  }
