import os
import dataclasses

import httpx

success = True

http = httpx.Client(headers={
	"Accept": "application/vnd.github.v3+json",
})

token = os.getenv("GITHUB_TOKEN")
if token:
	http.headers.update({
		"Authorization": f"token {token}"
	})


def log(lib_name: str, current_version: str, latest_version: str):
	if current_version == latest_version:
		print(f" {lib_name} is up-to-date ({current_version})")
	else:
		print(f" {lib_name} {latest_version} is available (current is {current_version})")
		global success
		success = False


@dataclasses.dataclass
class DepInfo:
	git_repository: str
	git_tag: str

	@staticmethod
	def from_file(path: str):
		info = DepInfo(git_repository="", git_tag="")
		with open(path, "r") as file:
			for line in file:
				if "GIT_REPOSITORY" in line:
					info.git_repository = line[line.index("GIT_REPOSITORY") + 15:].strip()
				elif "GIT_TAG" in line:
					info.git_tag = line[line.index("GIT_TAG") + 8:].strip()
		return info


@dataclasses.dataclass
class RepoInfo:
	full_name: str

	@staticmethod
	def get(repo_name: str) -> RepoInfo:
		repo = http.get(f"https://api.github.com/repos/{repo_name}").json()
		return RepoInfo(full_name=repo["full_name"])


def is_commit(git_tag: str) -> bool:
	if len(git_tag) != 40:
		return False
	try:
		int(git_tag, 16)
		return True
	except ValueError:
		return False


def latest_git_ref(repo_name: str) -> str:
	refs = http.get(f"https://api.github.com/repos/{repo_name}/git/refs").json()
	return refs[0]["object"]["sha"]


def latest_tag(repo_name: str) -> str:
	tags = http.get(f"https://api.github.com/repos/{repo_name}/tags").json()
	return tags[0]["name"]


for file in os.listdir("deps"):
	info = DepInfo.from_file(os.path.join("deps", file))
	if len(info.git_repository) <= 0:
		continue
	full_name = info.git_repository[19:len(info.git_repository) - 4]
	latest_version = latest_git_ref(full_name) if is_commit(info.git_tag) else latest_tag(full_name)
	log(full_name, info.git_tag, latest_version)

http.close()
exit(0 if success else 1)
