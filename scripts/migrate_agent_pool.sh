#!/bin/bash

##########################################################
# Script to automate migration of agent pools across multiple git branches
# in all submodules of the sonic-buildimage repository.
# Usage: ./migrate_agent_pool.sh <source_pool:target_pool> <source_pool:target_pool>... <branch1> <branch2> ...
# Example: ./migrate_agent_pool.sh pool1:poolA pool2:poolB master 202505 202411
# Every argument with a colon (:) is treated as a pool replacement,
# and every other argument is treated as a branch name.
# This will create PRs in each submodule repository for the specified branches
# with the agent pool names replaced as specified.
##########################################################

set -e
mkdir -p /tmp/logs
TMP_DIR=$(mktemp -d)

GITHUB_USER="${GITHUB_USER:-mssonicbld}"
COMMIT_MSG="Automated agent pool migration"
PR_TITLE="Automated agent pool migration"
PR_BODY="This PR is created for automated agent pool migration across branches."

PR_BODY+="
Agent pools to be migrated:
"
echo "Agent pools to be migrated:"
for replacement in $POOL_MAPPING; do
    OLD="${replacement%%:*}"
    NEW="${replacement##*:}"
    echo "  - ${OLD} -> ${NEW}"
    PR_BODY+="- ${OLD} -> ${NEW}"$'\n'
done

PR_BODY+="
Branches processed:
"
echo "Branches to be processed:"
for branch in $BRANCHES; do
    echo "  - ${branch}"
    PR_BODY+="- ${branch}"$'\n'
done

replace_in_files() {
    
    repo="$1"
    # iterate only yaml files that are pipelines related and can be tracked by git
    targets=("azure-pipelines" ".azure-pipelines" "azure-pipelines.yml" "azurepipeline.yml") # folders or files to check

    git clone https://$GITHUB_USER:$TOKEN@github.com/$repo "$TMP_DIR/${repo##*/}" || { echo "Failed to clone repository: $repo"; return 1; }
    cd "$TMP_DIR/${repo##*/}" || { echo "Failed to change directory to temp repo dir"; return 1; }
    echo "Migrating agent pools in files under $repo"

    for replacement in $POOL_MAPPING; do
        OLD="${replacement%%:*}"
        NEW="${replacement##*:}"
        find "$target[@]" -type f \( -name "*.yml" -o -name "*.yaml" \) | while read -r file; do
            if grep -q "${OLD}" "$file"; then
                if sed -i.bak "s/${OLD}/${NEW}/g" "$file"; then
                    rm -f "${file}.bak"
                    echo "Updated ${file}: ${OLD} -> ${NEW}"
                else
                    echo "Failed to update ${file}: ${OLD} -> ${NEW}"
                fi
            fi
        done
    done
    cd -
}
    

process_repo() {
    local repo="$1"
    REPO_BASENAME="${repo##*/}"
    
    for skip in $SKIP_REPOS; do
        if [ "${repo}" == "${skip}" ]; then
            echo "\n============= Skipping repository: ${repo} ============="
            return 0
        fi
    done

    echo "\n============= Processing repository: ${repo} ============="

    if ! gh repo view "${GITHUB_USER}/${REPO_BASENAME}" > /dev/null 2>&1; then
        echo "Forking repository ${repo} to user ${GITHUB_USER}"
        gh repo fork repo --clone=false 
    fi

    if ! git remote | grep -q "fork"; then
        git remote add fork "https://github.com/${GITHUB_USER}/${REPO_BASENAME}.git"
    fi

    git fetch origin
    git fetch fork

    echo "${repo}" >> /tmp/logs/migration_results.log

    for branch in $BRANCHES; do

        echo "=== Processing branch [${branch}] for ${repo} ==="

        if git show-ref --verify --quiet "refs/remotes/origin/${branch}"; then
            git checkout "origin/${branch}"
        else
            echo "Branch ${branch} does not exist in ${repo}, skipping."
            continue
        fi
        git pull origin "$branch" 
        NEW_BRANCH="migrate-agent-pool-${branch}"
        if git show-ref --verify --quiet "refs/remotes/fork/${NEW_BRANCH}"; then
            git branch -D "${NEW_BRANCH}"
        fi
        git checkout -b "${NEW_BRANCH}"

        replace_in_files "${GITHUB_USER}/${REPO_BASENAME}"

        git -C "$repo_path" diff --name-only --diff-filter=M | xargs -r git -C "$repo_path" add
        if [ -n "$(git -C "$repo_path" diff --cached --name-only)" ]; then
            
            git commit -s -m "${COMMIT_MSG}"
            git push fork "${NEW_BRANCH}" -f

            echo "Creating PR for branch ${branch} in repository ${repo}"

            if [ "$(gh pr list --repo "${repo}" --head "${NEW_BRANCH}" --base "${branch}" --json number --jq 'length')" -eq 0 ]; then
                PR_TITLE_BRANCH="${PR_TITLE} for branch ${branch}"
                PR_URL=$(gh pr create \
                                --repo "${repo}" \
                                --head "${GITHUB_USER}:${NEW_BRANCH}" \
                                --base "${branch}" \
                                --title "${PR_TITLE_BRANCH}" \
                                --body "${PR_BODY}" \
                                2>&1 | grep -Eo 'https://github\.com/[^ ]+')
                echo "PR created for branch ${branch} in repository ${repo}: ${PR_URL}"
                echo "[PR created][${branch}]: ${PR_URL}" >>  /tmp/logs/migration_results.log
            else
                PR_URL=$(gh pr list --repo "${repo}" --head "${NEW_BRANCH}" --base "${branch}" --json url --jq '.[0].url')
                echo "A PR already exists for branch ${NEW_BRANCH} in repository ${repo}. PR updated."
                echo "[PR updated][${branch}]: ${PR_URL}" >> /tmp/logs/migration_results.log
            fi
            MODIFIED_REPOS+=("${repo}-${branch}")
        else
            echo "No changes detected in branch ${branch} of repository ${repo}"
        fi
    done

    echo "========================================\n" >> /tmp/logs/migration_results.log
    cd -
}

for repo in $TARGET_REPOS; do
    process_repo "$repo"
done
echo "All repos processed."
echo "PRs created in the following repositories:"
for repo in "${MODIFIED_REPOS[@]}"; do  
    echo " - ${repo}"
done
rm -rf "$TMP_DIR"