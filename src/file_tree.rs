use std::fs::{self};

enum FileTreeNodeType {
    Directory {
        children: Option<Vec<FileTreeNode>>,
    },
    File,
}

struct FileTreeNode {
    name: String,
    node_type: FileTreeNodeType,
}

pub struct FileTree {
    dir: String,
    root: FileTreeNode,
}

impl FileTree {
    pub fn new_in_working_dir() -> Self {
        const WORKING_DIR: &str = "./";
        let paths = fs::read_dir(WORKING_DIR).unwrap();

        let nodes = paths.into_iter()
            .map(|p| {
                let item = p.unwrap();
                let file_type = item.file_type().unwrap();
                let node_type = if file_type.is_dir() {
                    FileTreeNodeType::Directory {
                        children: None,
                    }
                } else {
                    FileTreeNodeType::File
                };

                FileTreeNode {
                    name: item.file_name().to_str().unwrap().to_owned(),
                    node_type,
                }
            })
            .collect::<Vec<FileTreeNode>>();

        Self {
            dir: WORKING_DIR.to_owned(),
            root: FileTreeNode {
                name: WORKING_DIR.to_owned(),
                node_type: FileTreeNodeType::Directory {
                    children: Some(nodes),
                },
            },
        }
    }

    pub fn print(&self) {
        if let FileTreeNodeType::Directory { children: Some(ref children), .. } = self.root.node_type {
            for child in children {
                println!("{}", child.name);
            }
        } else {
            panic!("Incorrect construction of root of file tree");
        }
    }
}
