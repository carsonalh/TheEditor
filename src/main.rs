use std::{ alloc, ptr, mem};

use file_tree::FileTree;
use windows::{
    core::*,
    Win32::{
        Foundation::*, Graphics::{Direct2D::{ID2D1Factory, *}, Gdi::{InvalidateRect, ValidateRect}}, System::LibraryLoader::*, UI::WindowsAndMessaging::*,
    },
};
use Common::{D2D1_COLOR_F, D2D_RECT_F, D2D_SIZE_U};

pub mod file_tree;

struct Window {
    hwnd: HWND,
    render_target: ID2D1HwndRenderTarget,
    square_brush: ID2D1SolidColorBrush,
    width: f32,
    height: f32,
    file_tree: file_tree::FileTree,
}

impl Window {
    fn new(name: PCWSTR) -> Result<Box<Window>> {
        unsafe {
            let class_name = w!("RustWindowClass");
            let h_instance = GetModuleHandleW(None)?;
            let wnd_class = WNDCLASSW {
                hInstance: h_instance.into(),
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
                None,
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

            let square_brush = render_target.CreateSolidColorBrush(&D2D1_COLOR_F {
                r: 0.1, g: 0.9, b: 0.1, a: 1.0
            }, None)?;

            let mut client_rect = RECT::default();
            GetClientRect(hwnd, &mut client_rect)?;

            let file_tree = FileTree::new_in_working_dir();

            ptr::write(window_ptr, Window {
                hwnd,
                render_target,
                square_brush,
                width: (client_rect.right - client_rect.left) as f32,
                height: (client_rect.bottom - client_rect.top) as f32,
                file_tree,
            });

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
            r: 0.3, g: 0.2, b: 0.9, a: 1.0
        }));

        const size: f32 = 100f32;

        self.render_target.FillRectangle(&D2D_RECT_F {
            left: self.width * 0.5 - size * 0.5,
            right: self.width * 0.5 + size * 0.5,
            top: self.height * 0.5 - size * 0.5,
            bottom: self.height * 0.5 + size * 0.5,
        }, &self.square_brush);

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
            let window_ptr = create_struct.as_ref().expect("lParam ptr for WM_CREATE should not be null").lpCreateParams as *mut Window;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, mem::transmute(window_ptr));

            return LRESULT(0)
        }

        let window_ptr: *mut Window = mem::transmute(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        let window = match window_ptr.as_mut() {
            Some(w) => w,
            None => {
                return DefWindowProcW(hwnd, message, wparam, lparam);
            },
        };

        match message {
            WM_CLOSE => {
                LRESULT(match DestroyWindow(hwnd) {
                    Ok(_) => 0,
                    _ => 1,
                })
            },
            WM_SIZE => {
                let width: u32 = (lparam.0 & 0xffff) as u32;
                let height: u32 = ((lparam.0 & 0xffff0000) >> 16) as u32;
                window.render_target.Resize(&D2D_SIZE_U { width, height }).unwrap();
                _ = InvalidateRect(Some(hwnd), None, false);
                window.width = width as f32;
                window.height = height as f32;
                LRESULT(0)
            },
            WM_PAINT => {
                window.paint().unwrap();
                _ = ValidateRect(Some(hwnd), None);
                LRESULT(0)
            },
            WM_QUIT | WM_DESTROY => {
                PostQuitMessage(0);
                LRESULT(0)
            },
            _ => {
                DefWindowProcW(hwnd, message, wparam, lparam)
            }
        }
    }
}

fn main() -> Result<()> {
    let mut window = Window::new(w!("Direct2D Rust Window"))?;
    window.run()?;

    Ok(())
}
