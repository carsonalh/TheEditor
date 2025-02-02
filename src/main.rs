use std::{alloc, mem, ptr};

use file_tree::{FileTree, FileTreeItemType};
use lipsum::lipsum;
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
        UI::Input::KeyboardAndMouse::*,
        UI::WindowsAndMessaging::*,
    },
};
use Common::{D2D1_COLOR_F, D2D_RECT_F, D2D_SIZE_U};

pub mod file_tree;

trait AsUtf16 {
    fn as_utf16<'a>(&self, buffer: &'a mut Vec<u16>) -> &'a [u16];
}

impl AsUtf16 for String {
    fn as_utf16<'a>(&self, buffer: &'a mut Vec<u16>) -> &'a [u16] {
        buffer.clear();
        for c in self.as_bytes() {
            buffer.push(*c as u16);
        }

        return buffer.as_slice();
    }
}

struct Window {
    hwnd: HWND,
    direct_2d_factory: ID2D1Factory,
    render_target: ID2D1HwndRenderTarget,
    text_brush: ID2D1SolidColorBrush,
    hover_brush: ID2D1SolidColorBrush,
    direct_write_factory: IDWriteFactory,
    text_format_regular: IDWriteTextFormat,
    text_format_bold: IDWriteTextFormat,
    text_layout: IDWriteTextLayout,
    width: u32,
    height: u32,
    contents: String,
    cursor_pos: u32,
    contents_utf16: Vec<u16>,
    mouse_position: Option<(u32, u32)>,
}

impl Window {
    unsafe fn new(name: PCWSTR) -> Result<Box<Window>> {
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

        let direct_2d_factory: ID2D1Factory =
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, None)?;
        let render_target = direct_2d_factory.CreateHwndRenderTarget(
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
            24f32,
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

        let mut contents_utf16 = Vec::new();
        let contents_str = String::from(lipsum(200));

        let contents = contents_str.as_utf16(&mut contents_utf16);

        let mut window_rect: RECT = Default::default();
        GetClientRect(hwnd, &mut window_rect)?;

        let window_width = (window_rect.right - window_rect.left) as f32;
        let window_height = (window_rect.bottom - window_rect.top) as f32;

        let text_layout = direct_write_factory.CreateTextLayout(
            contents,
            &text_format_regular,
            window_width,
            window_height,
        )?;

        let mut client_rect = RECT::default();
        GetClientRect(hwnd, &mut client_rect)?;

        ptr::write(
            window_ptr,
            Window {
                hwnd,
                render_target,
                direct_2d_factory,
                text_brush,
                hover_brush,
                width: (client_rect.right - client_rect.left) as u32,
                height: (client_rect.bottom - client_rect.top) as u32,
                direct_write_factory,
                contents: contents_str,
                contents_utf16,
                cursor_pos: 0,
                text_format_regular,
                text_format_bold,
                text_layout,
                mouse_position: None,
            },
        );

        Ok(Box::from_raw(window_ptr))
    }

    unsafe fn run(&mut self) -> Result<()> {
        _ = ShowWindow(self.hwnd, SW_NORMAL);

        let mut message = MSG::default();

        while GetMessageW(&mut message, Some(self.hwnd), 0, 0).0 > 0 {
            _ = TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        Ok(())
    }

    unsafe fn paint(&mut self) -> Result<()> {
        self.render_target.BeginDraw();
        self.render_target.Clear(Some(&D2D1_COLOR_F {
            r: 0.0,
            g: 0.0,
            b: 0.0,
            a: 1.0,
        }));

        self.render_target.DrawTextLayout(
            Default::default(),
            &self.text_layout,
            &self.text_brush,
            Default::default(),
        );
        let mut x = 0f32;
        let mut y = 0f32;
        let mut metrics = DWRITE_HIT_TEST_METRICS::default();
        self.text_layout.HitTestTextPosition(
            self.cursor_pos,
            false,
            &mut x,
            &mut y,
            &mut metrics,
        )?;
        const EPSILON: f32 = 0.000001f32;
        const WIDTH: f32 = 3f32;
        if x.abs() < EPSILON {
            x += WIDTH / 2f32;
        }

        self.render_target.FillRectangle(&D2D_RECT_F {
            left: x - WIDTH / 2f32,
            right: x + WIDTH / 2f32,
            top: y,
            bottom: y + metrics.height,
        }, &self.text_brush);

        self.render_target.EndDraw(None, None)?;
        Ok(())
    }

    unsafe fn resize(&mut self, width: u32, height: u32) -> Result<()> {
        self.render_target
            .Resize(&D2D_SIZE_U { width, height })
            .unwrap();
        _ = InvalidateRect(Some(self.hwnd), None, false);

        self.width = width;
        self.height = height;

        _ = mem::replace(
            &mut self.text_layout,
            self.direct_write_factory.CreateTextLayout(
                self.contents.as_utf16(&mut self.contents_utf16),
                &self.text_format_regular,
                width as f32,
                height as f32,
            )?,
        );

        Ok(())
    }

    unsafe extern "system" fn window_proc(
        hwnd: HWND,
        message: u32,
        wparam: WPARAM,
        lparam: LPARAM,
    ) -> LRESULT {
        if message == WM_CREATE {
            let create_struct = lparam.0 as *const CREATESTRUCTW;
            let create_struct = create_struct
                .as_ref()
                .expect("lParam ptr for WM_CREATE should not be null");
            let window_ptr = create_struct.lpCreateParams as *mut Window;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, mem::transmute(window_ptr));

            return LRESULT(0);
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
                window.resize(width, height);
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
            }
            WM_KEYDOWN => {
                let mut handled = true;
                match VIRTUAL_KEY(wparam.0 as u16) {
                    VK_LEFT => window.cursor_pos = window.cursor_pos.saturating_sub(1),
                    VK_RIGHT => window.cursor_pos += 1,
                    _ => handled = false,
                };
                if handled {
                    _ = InvalidateRect(Some(hwnd), None, false);
                }
                LRESULT(0)
            }
            WM_QUIT | WM_DESTROY => {
                PostQuitMessage(0);
                LRESULT(0)
            }
            _ => DefWindowProcW(hwnd, message, wparam, lparam),
        }
    }
}

fn main() -> Result<()> {
    let mut window = unsafe { Window::new(w!("Direct2D Rust Window"))? };
    unsafe { window.run()? };

    Ok(())
}
