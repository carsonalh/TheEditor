use std::{
    default,
    fs::{self},
};

#[derive(Default)]
pub enum FileTreeItemType {
    Directory {
        open: bool,
        explored: bool,
    },
    #[default]
    File,
}

#[derive(Default)]
pub struct FileTreeItem {
    pub name: String,
    pub depth: u32,
    pub item_type: FileTreeItemType,
}

pub struct FileTree {
    items: Vec<FileTreeItem>,
}

impl FileTree {
    pub fn new_in_working_dir() -> Self {
        const WORKING_DIR: &str = "./";
        let paths = fs::read_dir(WORKING_DIR).unwrap();

        let items = paths
            .into_iter()
            .map(|p| {
                let item = p.unwrap();
                let file_type = item.file_type().unwrap();
                let item_type = if file_type.is_dir() {
                    FileTreeItemType::Directory {
                        open: false,
                        explored: false,
                    }
                } else {
                    FileTreeItemType::File
                };

                FileTreeItem {
                    name: item.file_name().to_str().unwrap().to_owned(),
                    depth: 0,
                    item_type,
                }
            })
            .collect::<Vec<FileTreeItem>>();

        Self { items }
    }

    pub fn display_iter<'a>(&'a self) -> FileTreeDisplayIterator<'a> {
        FileTreeDisplayIterator {
            index: 0usize,
            file_tree: self,
        }
    }
}

pub struct FileTreeDisplayIterator<'a> {
    index: usize,
    file_tree: &'a FileTree,
}

impl<'a> Iterator for FileTreeDisplayIterator<'a> {
    type Item = &'a FileTreeItem;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.file_tree.items.len() {
            return None;
        }

        let v = &self.file_tree.items[self.index];

        self.index += 1;

        while self.index < self.file_tree.items.len()
            && self.file_tree.items[self.index].depth > v.depth
        {
            self.index += 1;
        }

        Some(v)
    }
}
