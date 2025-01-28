use std::{alloc, mem, ptr};

use file_tree::{FileTree, FileTreeItemType};
use windows::{
    core::*,
    Win32::{
        Foundation::*,
        Graphics::DirectWrite::*,
        Graphics::{
            Direct2D::{ID2D1Factory, *},
            Gdi::{InvalidateRect, ValidateRect},
        },
        System::LibraryLoader::*,
        UI::WindowsAndMessaging::*,
    },
};
use Common::{D2D1_COLOR_F, D2D_RECT_F, D2D_SIZE_U};

pub mod file_tree;

struct Window {
    hwnd: HWND,
    render_target: ID2D1HwndRenderTarget,
    text_brush: ID2D1SolidColorBrush,
    hover_brush: ID2D1SolidColorBrush,
    text_format_regular: IDWriteTextFormat,
    text_format_bold: IDWriteTextFormat,
    width: u32,
    height: u32,
    file_tree: file_tree::FileTree,
    mouse_position: Option<(u32, u32)>,
}

impl Window {
    fn new(name: PCWSTR) -> Result<Box<Window>> {
        unsafe {
            let class_name = w!("RustWindowClass");
            let h_instance: HINSTANCE = GetModuleHandleW(None)?.into();
            let wnd_class = WNDCLASSW {
                hInstance: h_instance,
                lpszClassName: class_name,
                lpfnWndProc: Some(Self::window_proc),
                style: CS_HREDRAW | CS_VREDRAW,
                ..Default::default()
            };
            RegisterClassW(&wnd_class);

            let window_ptr = alloc::alloc(alloc::Layout::new::<Self>()) as *mut Window;

            let hwnd = CreateWindowExW(
                WINDOW_EX_STYLE::default(),
                class_name,
                name,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                None,
                None,
                Some(h_instance),
                Some(window_ptr as *const core::ffi::c_void),
            )?;

            let factory: ID2D1Factory = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, None)?;
            let render_target = factory.CreateHwndRenderTarget(
                &D2D1_RENDER_TARGET_PROPERTIES::default(),
                &D2D1_HWND_RENDER_TARGET_PROPERTIES {
                    hwnd: hwnd,
                    ..Default::default()
                },
            )?;

            let text_brush = render_target.CreateSolidColorBrush(
                &D2D1_COLOR_F {
                    r: 1.0,
                    g: 1.0,
                    b: 1.0,
                    a: 1.0,
                },
                None,
            )?;

            let hover_brush = render_target.CreateSolidColorBrush(
                &D2D1_COLOR_F {
                    r: 0.2,
                    g: 0.2,
                    b: 0.2,
                    a: 1.0,
                },
                None,
)?;

            let direct_write_factory =
                DWriteCreateFactory::<IDWriteFactory>(DWRITE_FACTORY_TYPE_ISOLATED)?;

            let text_format_regular = direct_write_factory.CreateTextFormat(
                w!("C:\\Windows\\Fonts\\segoeui.ttf"),
                None,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                32f32,
                w!("en-au"),
            )?;

            let text_format_bold = direct_write_factory.CreateTextFormat(
                w!("C:\\Windows\\Fonts\\segoeui.ttf"),
                None,
                DWRITE_FONT_WEIGHT_BOLD,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                32f32,
                w!("en-au"),
            )?;

            let mut client_rect = RECT::default();
            GetClientRect(hwnd, &mut client_rect)?;

            ptr::write(
                window_ptr,
                Window {
                    hwnd,
                    render_target,
                    text_brush,
                    hover_brush,
                    width: (client_rect.right - client_rect.left) as u32,
                    height: (client_rect.bottom - client_rect.top) as u32,
                    file_tree: FileTree::new_in_working_dir(),
                    text_format_regular,
                    text_format_bold,
                    mouse_position: None,
                },
            );

            Ok(Box::from_raw(window_ptr))
        }
    }

    fn run(&mut self) -> Result<()> {
        unsafe {
            _ = ShowWindow(self.hwnd, SW_NORMAL);

            let mut message = MSG::default();

            while GetMessageW(&mut message, Some(self.hwnd), 0, 0).0 > 0 {
                _ = TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            Ok(())
        }
    }

    unsafe fn paint(&self) -> Result<()> {
        self.render_target.BeginDraw();
        self.render_target.Clear(Some(&D2D1_COLOR_F {
            r: 0.3,
            g: 0.2,
            b: 0.9,
            a: 1.0,
        }));

        let height = 50f32;
        let left = 0f32;
        let right = 500f32;
        let mut top = 0f32;
        let mut bottom = height;

        for item in self.file_tree.display_iter() {
            let text_format = match item.item_type {
                FileTreeItemType::File => &self.text_format_regular,
                FileTreeItemType::Directory { .. } => &self.text_format_bold,
            };

            if let Some(pos) = self.mouse_position {
                let x = pos.0 as f32;
                let y = pos.1 as f32;

                if left < x && x < right && top < y && y < bottom {
                    self.render_target.FillRectangle(&D2D_RECT_F {
                        left,
                        right,
                        bottom,
                        top,
                    }, &self.hover_brush);
                }
            }

            self.render_target.DrawText(
                item.name
                    .as_bytes()
                    .iter()
                    .map(|byte| *byte as u16)
                    .collect::<Box<[u16]>>()
                    .as_ref(),
                text_format,
                &D2D_RECT_F {
                    left,
                    right,
                    bottom,
                    top,
                },
                &self.text_brush,
                D2D1_DRAW_TEXT_OPTIONS::default(),
                DWRITE_MEASURING_MODE::default(),
            );

            top += height;
            bottom += height;
        }

        self.render_target.EndDraw(None, None)?;
        Ok(())
    }

    unsafe extern "system" fn window_proc(
        hwnd: HWND,
        message: u32,
        wparam: WPARAM,
        lparam: LPARAM,
    ) -> LRESULT {
        if message == WM_CREATE {
            let create_struct: *const CREATESTRUCTW = mem::transmute(lparam.0);
            let create_struct = create_struct.as_ref().expect("lParam ptr for WM_CREATE should not be null");
            let window_ptr = create_struct.lpCreateParams as *mut Window;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, mem::transmute(window_ptr));

            return LRESULT(0)
        }

        let window_ptr: *mut Window = mem::transmute(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        let window = match window_ptr.as_mut() {
            Some(w) => w,
            None => {
                return DefWindowProcW(hwnd, message, wparam, lparam);
            }
        };

        match message {
            WM_CLOSE => LRESULT(match DestroyWindow(hwnd) {
                Ok(_) => 0,
                _ => 1,
            }),
            WM_SIZE => {
                let width: u32 = (lparam.0 & 0xffff) as u32;
                let height: u32 = ((lparam.0 & 0xffff0000) >> 16) as u32;
                window
                    .render_target
                    .Resize(&D2D_SIZE_U { width, height })
                    .unwrap();
                _ = InvalidateRect(Some(hwnd), None, false);
                window.width = width;
                window.height = height;
                LRESULT(0)
            }
            WM_PAINT => {
                window.paint().unwrap();
                _ = ValidateRect(Some(hwnd), None);
                LRESULT(0)
            }
            WM_MOUSEMOVE => {
                let mouse_x = (lparam.0 & 0xffff) as u32;
                let mouse_y = ((lparam.0 & 0xffff0000) >> 16) as u32;

                window.mouse_position = Some((mouse_x, mouse_y));

                _ = InvalidateRect(Some(hwnd), None, false);

                LRESULT(0)
            },
            WM_QUIT | WM_DESTROY => {
                PostQuitMessage(0);
                LRESULT(0)
            }
            _ => DefWindowProcW(hwnd, message, wparam, lparam),
        }
    }
}

fn main() -> Result<()> {
    let mut window = Window::new(w!("Direct2D Rust Window"))?;
    window.run()?;

    Ok(())
}
