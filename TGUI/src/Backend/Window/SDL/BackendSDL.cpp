/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TGUI - Texus' Graphical User Interface
// Copyright (C) 2012-2021 Bruno Van de Velde (vdv_b@tgui.eu)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <TGUI/Backend/Window/SDL/BackendSDL.hpp>

#include <SDL.h>
#include <SDL_syswm.h>

#ifdef TGUI_SYSTEM_WINDOWS
    #include <TGUI/WindowsInclude.hpp>
#endif

#if defined(TGUI_SYSTEM_LINUX) && defined(TGUI_USE_X11)
    #include <X11/Xlib.h>
    #include <X11/cursorfont.h>
#endif

#ifdef TGUI_SYSTEM_ANDROID
    #include <jni.h>
    #include <android/asset_manager.h>
    #include <android/asset_manager_jni.h>
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace tgui
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    BackendSDL::~BackendSDL()
    {
        for (auto& cursor : m_mouseCursors)
        {
            if (cursor.second)
                SDL_FreeCursor(cursor.second);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::setGuiWindow(BackendGui* gui, SDL_Window* window)
    {
        TGUI_ASSERT(m_guis.find(gui) != m_guis.end(), "BackendSDL::setGuiWindow called with a gui that wasn't attached");
        m_guis[gui].window = window;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::attachGui(BackendGui* gui)
    {
        m_guis[gui] = {};
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::detatchGui(BackendGui* gui)
    {
        // Don't check if it existed, detach is called for every gui while attached is only called for properly initialized guis
        m_guis.erase(gui);

        if (m_destroyOnLastGuiDetatch && m_guis.empty())
            setBackend(nullptr);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::setMouseCursorStyle(Cursor::Type type, const std::uint8_t* pixels, Vector2u size, Vector2u hotspot)
    {
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(const_cast<std::uint8_t*>(pixels), static_cast<int>(size.x), static_cast<int>(size.y),
            32, 4 * static_cast<int>(size.x), 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
        if (!surface)
            return;

        SDL_Cursor* bitmapCursor = SDL_CreateColorCursor(surface, static_cast<int>(hotspot.x), static_cast<int>(hotspot.y));
        SDL_FreeSurface(surface);

        if (!bitmapCursor)
            return;

        updateMouseCursorStyle(type, bitmapCursor);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::resetMouseCursorStyle(Cursor::Type type)
    {
#ifdef TGUI_SYSTEM_LINUX
        // On Linux we use directional resize arrows, but SDL has no support for them
        if ((type == Cursor::Type::SizeLeft) || (type == Cursor::Type::SizeRight)
         || (type == Cursor::Type::SizeTop) || (type == Cursor::Type::SizeBottom)
         || (type == Cursor::Type::SizeBottomRight) || (type == Cursor::Type::SizeTopLeft)
         || (type == Cursor::Type::SizeBottomLeft) || (type == Cursor::Type::SizeTopRight))
        {
            // If the cursor was previously set to a bitmap then release its resources
            auto it = m_mouseCursors.find(type);
            if ((it != m_mouseCursors.end()) && it->second)
            {
                SDL_FreeCursor(it->second);
                m_mouseCursors.erase(it);
            }

            updateMouseCursorStyle(type, nullptr);
            return;
        }
#endif

        updateMouseCursorStyle(type, createSystemCursor(type));
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::setMouseCursor(BackendGui* gui, Cursor::Type type)
    {
        TGUI_ASSERT(m_guis.find(gui) != m_guis.end(), "BackendSDL::setMouseCursor called with a gui that wasn't attached");
        if (type == m_guis[gui].mouseCursor)
            return;

        m_guis[gui].mouseCursor = type;

        // If the gui has no access to the window then we can't change the mouse cursor
        if (!m_guis[gui].window)
            return;

        updateShownMouseCursor(m_guis[gui].window, type);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::openVirtualKeyboard(const FloatRect& rect)
    {
        SDL_Rect inputRect;
        inputRect.x = static_cast<int>(rect.left);
        inputRect.y = static_cast<int>(rect.top);
        inputRect.w = static_cast<int>(rect.width);
        inputRect.h = static_cast<int>(rect.height);

        SDL_StartTextInput();
        SDL_SetTextInputRect(&inputRect);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::closeVirtualKeyboard()
    {
        SDL_StopTextInput();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool BackendSDL::isKeyboardModifierPressed(Event::KeyModifier modifierKey)
    {
        const SDL_Keymod pressedModifiers = SDL_GetModState();
        switch (modifierKey)
        {
        case Event::KeyModifier::System:
            return (pressedModifiers & KMOD_GUI) != 0;
        case Event::KeyModifier::Control:
            return (pressedModifiers & KMOD_CTRL) != 0;
        case Event::KeyModifier::Shift:
            return (pressedModifiers & KMOD_SHIFT) != 0;
        case Event::KeyModifier::Alt:
            return (pressedModifiers & KMOD_ALT) != 0;
        }

        TGUI_ASSERT(false, "BackendSDL::isKeyboardModifierPressed called with an invalid value");
        return false;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::setClipboard(const String& contents)
    {
#ifdef TGUI_SYSTEM_WINDOWS
        // If setting the clipboard fails on Windows then sleep a moment and try again
        if (SDL_SetClipboardText(contents.toStdString().c_str()) < 0)
        {
            Sleep(1);
            SDL_SetClipboardText(contents.toStdString().c_str());
        }
#else
        SDL_SetClipboardText(contents.toStdString().c_str());
#endif
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    String BackendSDL::getClipboard() const
    {
        String contents;
        char* text = SDL_GetClipboardText();
        if (text)
        {
            contents = text;
            SDL_free(text);
        }

        return contents;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef TGUI_SYSTEM_ANDROID
    std::unique_ptr<std::uint8_t[]> BackendSDL::readFileFromAndroidAssets(const String& filename, std::size_t& fileSize) const
    {
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        jclass clazz = env->GetObjectClass(activity);

        jmethodID methodID = env->GetMethodID(clazz, "getAssets", "()Landroid/content/res/AssetManager;");
        jobject assetManagerObject = env->CallObjectMethod(activity, methodID);
        jobject globalAssetManagerRef = env->NewGlobalRef(assetManagerObject);
        AAssetManager* assetManager = AAssetManager_fromJava(env, globalAssetManagerRef);
        if (!assetManager)
            return nullptr;

        AAsset* asset = AAssetManager_open(assetManager, filename.toStdString().c_str(), AASSET_MODE_UNKNOWN);
        if (!asset)
            return nullptr;

        const off_t assetLength = AAsset_getLength(asset);

        auto buffer = MakeUniqueForOverwrite<std::uint8_t[]>(assetLength);
        if (AAsset_read(asset, buffer.get(), assetLength) < 0)
            return nullptr;

        AAsset_close(asset);

        fileSize = static_cast<std::size_t>(assetLength);
        return buffer;
    }
#endif
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SDL_Cursor* BackendSDL::createSystemCursor(Cursor::Type type)
    {
        SDL_SystemCursor typeSDL = SDL_SYSTEM_CURSOR_ARROW;
        switch (type)
        {
        case Cursor::Type::Arrow:
            typeSDL = SDL_SYSTEM_CURSOR_ARROW;
            break;
        case Cursor::Type::Text:
            typeSDL = SDL_SYSTEM_CURSOR_IBEAM;
            break;
        case Cursor::Type::Hand:
            typeSDL = SDL_SYSTEM_CURSOR_HAND;
            break;
        case Cursor::Type::SizeLeft:
        case Cursor::Type::SizeRight:
            typeSDL = SDL_SYSTEM_CURSOR_SIZEWE;
            break;
        case Cursor::Type::SizeTop:
        case Cursor::Type::SizeBottom:
            typeSDL = SDL_SYSTEM_CURSOR_SIZENS;
            break;
        case Cursor::Type::SizeBottomRight:
        case Cursor::Type::SizeTopLeft:
            typeSDL = SDL_SYSTEM_CURSOR_SIZENWSE;
            break;
        case Cursor::Type::SizeBottomLeft:
        case Cursor::Type::SizeTopRight:
            typeSDL = SDL_SYSTEM_CURSOR_SIZENESW;
            break;
        case Cursor::Type::Crosshair:
            typeSDL = SDL_SYSTEM_CURSOR_CROSSHAIR;
            break;
        case Cursor::Type::Help:
            TGUI_PRINT_WARNING("BackendSDL doesn't support Cursor::Type::Help");
            break;
        case Cursor::Type::NotAllowed:
            typeSDL = SDL_SYSTEM_CURSOR_NO;
            break;
        }

        return SDL_CreateSystemCursor(typeSDL);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::updateMouseCursorStyle(Cursor::Type type, SDL_Cursor* cursor)
    {
#ifdef TGUI_SYSTEM_WINDOWS
        // Make sure the old cursor isn't still being used before we destroy it
        bool cursorInUse = false;
        for (auto& pair : m_guis)
        {
            if (pair.second.mouseCursor == type)
                cursorInUse = true;
        }
        if (cursorInUse)
            SetCursor(static_cast<HCURSOR>(LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED)));
#endif

        if (m_mouseCursors[type])
            SDL_FreeCursor(m_mouseCursors[type]);

        m_mouseCursors[type] = cursor;

        // Update the cursor on the screen if the cursor was in use
        for (auto& pair : m_guis)
        {
            if (pair.second.mouseCursor == type)
            {
                if (pair.second.window)
                    updateShownMouseCursor(pair.second.window, type);
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BackendSDL::updateShownMouseCursor(SDL_Window* window, Cursor::Type type)
    {
        TGUI_ASSERT(window != nullptr, "BackendSDL::updateShownMouseCursor requires a valid window");

#if defined(TGUI_SYSTEM_LINUX) && defined(TGUI_USE_X11)
        // On Linux we use directional resize arrows, but SDL has no support for them
        if ((type == Cursor::Type::SizeLeft) || (type == Cursor::Type::SizeRight)
            || (type == Cursor::Type::SizeTop) || (type == Cursor::Type::SizeBottom)
            || (type == Cursor::Type::SizeBottomRight) || (type == Cursor::Type::SizeTopLeft)
            || (type == Cursor::Type::SizeBottomLeft) || (type == Cursor::Type::SizeTopRight))
        {
            if (!m_mouseCursors[type]) // Only bypass SDL when system cursors are used
            {
                SDL_SysWMinfo sysInfo;
                SDL_VERSION(&sysInfo.version);
                if (SDL_GetWindowWMInfo(window, &sysInfo) && (sysInfo.subsystem == SDL_SYSWM_X11))
                {
                    auto* displayX11 = sysInfo.info.x11.display;
                    if (!displayX11)
                        return;

                    unsigned int shapeX11;
                    if (type == Cursor::Type::SizeLeft)
                        shapeX11 = XC_left_side;
                    else if (type == Cursor::Type::SizeRight)
                        shapeX11 = XC_right_side;
                    else if (type == Cursor::Type::SizeTop)
                        shapeX11 = XC_top_side;
                    else if (type == Cursor::Type::SizeBottom)
                        shapeX11 = XC_bottom_side;
                    else if (type == Cursor::Type::SizeBottomRight)
                        shapeX11 = XC_bottom_right_corner;
                    else if (type == Cursor::Type::SizeTopLeft)
                        shapeX11 = XC_top_left_corner;
                    else if (type == Cursor::Type::SizeBottomLeft)
                        shapeX11 = XC_bottom_left_corner;
                    else // if (type == Cursor::Type::SizeTopRight)
                        shapeX11 = XC_top_right_corner;

                    auto cursorX11 = XCreateFontCursor(displayX11, shapeX11);
                    if (cursorX11 != None)
                    {
                        XDefineCursor(displayX11, sysInfo.info.x11.window, cursorX11);
                        XFreeCursor(displayX11, cursorX11);
                    }

                    XFlush(displayX11);
                }

                return;
            }
        }
#else
        (void)window;
#endif

        // If the cursor doesn't exist yet then create it now
        if (!m_mouseCursors[type])
        {
            SDL_Cursor* cursor = createSystemCursor(type);
            if (!cursor)
                return;

            m_mouseCursors[type] = cursor;
        }

        // Pass the cursor to SDL to set it while the mouse is on top of the window
        SDL_SetCursor(m_mouseCursors[type]);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
