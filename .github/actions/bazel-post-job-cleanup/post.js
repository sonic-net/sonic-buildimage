const { spawnSync } = require("node:child_process");

if (!process.env.GITHUB_WORKSPACE || !process.env.STATE_cleanup_directory ||
    !process.env.STATE_cleanup_controller) {
    console.error("Bazel cleanup requires its registered checkout and job directory.");
    process.exit(1);
}
const args = ["-B", "-m", "tools.bazel.ci.cleanup_container_bazel",
    "--job-directory", process.env.STATE_cleanup_directory];
if (process.env.STATE_cleanup_migrated_job === "true") args.push("--migrated-job");
const result = spawnSync("python3", args, {
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
