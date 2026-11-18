const { spawnSync } = require("node:child_process");
const { appendFileSync } = require("node:fs");

const controller = process.env.SONIC_BAZEL_CLEANUP_CONTROLLER || process.env.GITHUB_WORKSPACE;
const migrated = process.env["INPUT_MIGRATED-JOB"] || "false";
if (!["true", "false"].includes(migrated)) {
    console.error("migrated-job must be true or false.");
    process.exit(1);
}
const args = ["-B", "-m", "tools.bazel.ci.cleanup_container_bazel", "--prepare"];
if (migrated === "true") args.push("--migrated-job");
const result = spawnSync("python3", args, {
    cwd: controller,
    encoding: "utf8",
    stdio: ["ignore", "pipe", "inherit"],
});
if (result.error || result.status !== 0) {
    console.error(result.error ? result.error.message : "Cannot register Bazel cleanup.");
    process.exit(result.status || 1);
}
const { directory } = JSON.parse(result.stdout);
if (typeof directory !== "string" || /[\r\n]/.test(directory) ||
    typeof controller !== "string" || /[\r\n]/.test(controller)) {
    console.error("Bazel cleanup paths must be single-line strings.");
    process.exit(1);
}
appendFileSync(process.env.GITHUB_ENV, `SONIC_BAZEL_JOB_TEMP=${directory}\n`);
appendFileSync(process.env.GITHUB_STATE, `cleanup_directory=${directory}\n`);
appendFileSync(process.env.GITHUB_STATE, `cleanup_controller=${controller}\n`);
appendFileSync(process.env.GITHUB_STATE, `cleanup_migrated_job=${migrated}\n`);
console.log("Registered Bazel cleanup after subsequent actions finish their post-job cache saves.");
