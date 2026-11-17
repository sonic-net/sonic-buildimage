const { spawnSync } = require("node:child_process");

if (!process.env.GITHUB_WORKSPACE || !process.env.STATE_cleanup_directory ||
    !process.env.STATE_cleanup_controller) {
    console.error("Bazel cleanup requires its registered checkout and job directory.");
    process.exit(1);
}
const result = spawnSync("python3", ["-B", "-m", "tools.bazel.ci.cleanup_container_bazel",
    "--job-directory", process.env.STATE_cleanup_directory], {
    cwd: process.env.STATE_cleanup_controller,
    stdio: "inherit",
});
if (result.error) {
    console.error(`Cannot start Bazel cleanup: ${result.error.message}`);
    process.exit(1);
}
if (result.status === null) {
    console.error(`Bazel cleanup terminated by signal ${result.signal}.`);
    process.exit(1);
}
process.exit(result.status);
