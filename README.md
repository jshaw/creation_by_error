# creation_by_error
creation_by_error source for OPEN show

In the git repo I updated the git working tree for settings.h so i can store local enviroment vars in the file locally but still have the file apart of the the repo...
http://stackoverflow.com/questions/4348590/how-can-i-make-git-ignore-future-revisions-to-a-file
`git update-index --skip-worktree settings.h`
`git update-index --no-skip-worktree settings.h`


