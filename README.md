# pkg-template

This repository serves as a template for creating Debian package repositories within the Qualcomm Linux ecosystem. It provides the essential structure, GitHub workflows, and configuration necessary to integrate with the [qcom-build-utils](https://github.com/qualcomm-linux/qcom-build-utils) repository, enabling standardized Debian package building processes.

For a comprehensive tutorial on utilizing this template with an example project, refer to the [pkg-example README](https://github.com/qualcomm-linux/pkg-example/blob/main/README.md).

## Quick Start

To create a new Debian package repository using this template:

1. Navigate to this repository's GitHub page and click the **"Use this template"** button located in the top right corner.
2. Name the new repository with the prefix `pkg-` to adhere to the naming convention for package repositories.
3. Ensure the **"Include all branches"** option is enabled. Otherwise by default, only the default branch "main" is cloned.

## Branches

- **main**: The primary branch containing workflow logic in the `.github/` folder, along with boilerplate documentation files such as license, contribution guidelines, and this README.
- **debian/qcom-next**: An orphan branch with unrelated history from main. It contains a debian/ folder with template files. Its just to give a starting point and structure. The first job for the user templating from this repo will be to update this debian/ folder. The information about the name **debian/qcom-next** and other naming conventions can be found [here](https://qualcomm-confluence.atlassian.net/wiki/spaces/LinuxCoreOS/pages/2879858691/pkg-+repository+specification)

## Workflows

The `main` branch includes the following workflows in the `.github/workflows/` directory:

- **qcom-preflight-checks.yml**: A sanity check workflow inherited from the base Qualcomm template.
- **stale-issues.yml**: A workflow for managing stale issues, also inherited from the base template.
- **build-debian-package.yml**: Builds the Debian package for this repository. This workflow serves as an entry point that invokes reusable workflows from the centralized qcom-build-utils repository.
- **promote-upstream.yml**: Promotes the package's tracking version to a new upstream release. This workflow also triggers reusable workflows in qcom-build-utils.

## Repository Configuration

### AWS arm64 runners

The workflows support running and compiling on both AMD64 and ARM64 architectures. When selecting 'debian-latest' as runnner, you are selecting 
AMD64 runners from gihub itself. Otherwise, if you select "lecore-prd-u2404-arm64-xlrg-od-ephem" as the runner, you are selecting one of the AWS ARM64 ephemeral runners which are not part of github, they are administered by Qualcomm and need to be enabled on a per-repo basis.
On teams, join the group called "AWS Runners" and ask about having the AWS runners enabled for your repo

### Repository Variables

Set the following repository variables to establish links between upstream and package repositories:

- **UPSTREAM_REPO_GITHUB_NAME**: In this package repo, this is the GitHub name of the upstream repository (e.g. in the case of the pkg-example, `qualcomm-linux/qcom-example-package-source`).
- **PKG_REPO_GITHUB_NAME**: This variable is set in the upstream project repo; the GitHub name of the package repository (e.g., `qualcomm-linux/pkg-example`).

### Branch Protection Rules

Configure branch protection for `debian/qcom-next`:

- Restrict deletions.
- Require pull requests before merging.
- Block force pushes.
- Add `build / build-debian-package` as a required status check.

### Additional Settings

- Ask [Mark Matyas](mmatyas@qti.qualcomm.com) to share the following org secret/variables with the repo :
  - secrets.DEB_PKG_BOT_CI_TOKEN
  - vars.DEB_PKG_BOT_CI_USERNAME
  - vars.DEB_PKG_BOT_CI_EMAIL
- Enable **"Automatically delete head branches"** for pull requests.
- Allow only merge commits for pull request merges.
- Enable **release immutability** in the upstream repository.
- Add the Qualcomm Github Service bot as a user with write access :
  - While the repo is private, add the Github user **qcom-service-bot**.
  - If/when the repo is made public, there will be a big change in how the contributors are handled. 
    After that, the contributors list is cleared, and one need to re-enroll as a contributor. The way
    to do that is completely different from when the repo was private. When it was private, the creator
    of the repo had the possibility to go into the repo settings and add whoever. When made public, the
    repo's maintainer/contributor list is managed by a Qualcomm internal mailing list. If the repo is
    named pkg-foo, then the Maintainer list will be named Maintainers.pkg-foo, and one need to request
    access via https://lists.qualcomm.com, find the list and ask access. For the bot, you must add it via
    its qualcomm username **githubservice**@qti.qualcomm.com, as opposed to its Github handle above.


## Making your pkg-repo to public

When your pkg-repo is mature enough, it will need to be made public.

### How to make the repo go public
 
The first step in making a repo go public is the Tradesmark/legal process.
Your POC (Person of Contact) in this step is [Stephanie Arce](sarce@qti.qualcomm.com)

You need to [complete an OSS Contribution Request](https://jira-dc4.qualcomm.com/jira/secure/CreateIssue.jspa?pid=46536&issuetype=13440)

Here is a bit of help on the mandatory fields when filling the form:

- Components : Select "Other"
- Summary : Give a short description such as "Debian packaging repository for X"
- Project name: This would be the repo name, such as "pkg-example"
- Destination URL: Give the Github URL, such as "www.github.com/qualcomm-linux/pkg-example"
- Description: Give a description about the repo being a debian packaging repo, which packages another upstream project X
- Project Licenses: Give the license of the repo. Unless something changes, it should remain  BSD-3-Clause License.
- Contribution plan: 
  - Project Description: You can give the same project description as above
  - What are ourr general plan with the Project: Talk about it being the debian packaging for project X
  - Do we have any Leadership positions in the project: Give yourself/manager
  - Is Qualcomm expanding the project by contributing technology implementations: Likely a simple no

More information can be found [here](https://github.qualcomm.com/pages/osdo/handbook/qcom-github/docs/new-project-checklist/)

Once submitted, it will generally take a couple of days to a week to get approval from Legal.

The, once you receive an email about the acceptance of the Legal team, you will have to submit a ticket about enabling the repo.

In this process, your POC will be [Mark Matyas](mmatyas@qti.qualcomm.com)

Complete the form : https://ossops.qualcomm.com/github/enable-repo/

### Note about Github Repo roles after going public

When creating a repo withing the organization, it always starts as a private repo and one need to go through an approval
process in order to have the repo go public. 

Upon creation of the repo, the person who created it gets to be assigned the maintainer responsability and has full control
over it in the settings. When the repo goes public, this changes. The repo creator stops having all the powers over the repo,
and in order to join the repo as a contributor, one needs to enroll via a github mailing list instead of directly through the repo
settings. 

This is the link to go to in order to ask to be a contributor or a maintainer of a Qualcomm publib github repo : 
https://lists.qualcomm.com/ListManager

In the searchbox, one need to find the contributor/maintainer list corresponding to the repo. 
If the repo is named "pkg-example", then the list(s) to search for are : 

- contributors.pkg-example
- maintainers.pkg-example

It takes about an hour between the list acceptance and the new role to reflect in Github





## Getting in Contact

For support or inquiries, contact sbeaudoi@qti.qualcomm.com.

## License

pkg-template is licensed under the [BSD-3-Clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.
