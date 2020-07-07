#include "AvoGUI.hpp"

#include "Font data.hpp"

//------------------------------

#include <array>
#include <stack>
#include <random>
#include <ctime>

#if __has_include("filesystem")
#include <filesystem>
namespace filesystem = std::filesystem;
#else
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#endif

//------------------------------

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#include <Windows.h>
#undef DrawTextW
#undef min
#undef max

#include <windowsx.h>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <Shlwapi.h>
#include <dwmapi.h>

#include <d2d1effects.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <comdef.h>

#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "Dwmapi")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "windowscodecs")
#endif

//------------------------------

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <libpng/png.h>
#include <jpeglib.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <unistd.h>
#include <cstdio>
#include <fstream>

#undef None
#endif

//------------------------------

using Avo::Index;
using Avo::Count;
using Avo::Range;
using Avo::Indices;

//------------------------------

namespace Console
{
	//
	// Private
	//

	auto Console::readString(std::string& p_string) -> void
	{
#ifdef _WIN32
		// The only way to read unicode characters from the console on Windows is to read UTF-16 in a wide string.
		wchar_t buffer[1 << 10]; // We don't know before how much is going to be read, so we use a large enough buffer (1kb).
		DWORD bufferSize;
		ReadConsoleW(m_inputHandle, buffer, std::size(buffer), &bufferSize, nullptr);
		bufferSize -= 2; // remove \r\n return characters

		// Because we use utf-8, convert it from UTF-16 to UTF-8 and store that in the output.
		auto outputSize = WideCharToMultiByte(CP_UTF8, 0, buffer, bufferSize, 0, 0, 0, 0);
		p_string.resize(outputSize);
		WideCharToMultiByte(CP_UTF8, 0, buffer, bufferSize, (char*)p_string.data(), outputSize, 0, 0);
#else
		// Most other platforms use UTF-8 by default.
		std::cin >> p_string;
#endif
	}

	//
	// Public
	//

	Console::Console()
	{
#ifdef _WIN32
		SetConsoleOutputCP(CP_UTF8);
		m_inputHandle = GetStdHandle(STD_INPUT_HANDLE);
#endif
	}
}

//------------------------------

auto randomEngine = std::mt19937_64{static_cast<unsigned long long>(time(nullptr))};
auto uniformDistribution = std::uniform_real_distribution{0.l, 1.l};
auto normalDistribution = std::normal_distribution{0.l, 1.l};

auto Avo::random() -> long double
{
	return uniformDistribution(randomEngine);
}
auto Avo::randomNormal() -> long double
{
	return normalDistribution(randomEngine);
}

//------------------------------

auto Avo::convertUtf8ToUtf16(std::string_view const p_input, Range<char16*> const p_output) -> void
{
#ifdef _WIN32
	auto const length = MultiByteToWideChar(
		CP_UTF8, 0, 
		p_input.data(), p_input.size(), 
		reinterpret_cast<wchar_t*>(p_output.data()), p_output.size()
	);
	p_output[length] = 0;
#endif
}
auto Avo::convertUtf8ToUtf16(std::string_view const p_input) -> std::u16string
{
#ifdef _WIN32
	auto result = std::u16string(MultiByteToWideChar(CP_UTF8, 0, p_input.data(), p_input.size(), 0, 0), '\0');
	MultiByteToWideChar(
		CP_UTF8, 0, 
		p_input.data(), p_input.size(), 
		reinterpret_cast<wchar_t*>(result.data()), result.size()
	);
	return result;
#endif
}
auto Avo::getNumberOfUnitsInUtfConvertedString(std::string_view const p_input) -> Count
{
#ifdef _WIN32
	return MultiByteToWideChar(CP_UTF8, 0, p_input.data(), p_input.size(), 0, 0);
#endif
}

auto Avo::convertUtf16ToUtf8(std::u16string_view const p_input, Range<char8*> const p_output) -> void
{
#ifdef _WIN32
	auto const length = WideCharToMultiByte(
		CP_UTF8, 0, 
		reinterpret_cast<wchar_t const*>(p_input.data()), p_input.size(), 
		p_output.data(), p_output.size(), 
		0, 0
	);
	p_output[length] = 0;
#endif
}
auto Avo::convertUtf16ToUtf8(std::u16string_view const p_input) -> std::string
{
#ifdef _WIN32
	auto result = std::string(WideCharToMultiByte(
		CP_UTF8, 0, 
		reinterpret_cast<wchar_t const*>(p_input.data()), p_input.size(), 
		0, 0, 0, 0
	), '\0');
	WideCharToMultiByte(
		CP_UTF8, 0, 
		reinterpret_cast<wchar_t const*>(p_input.data()), p_input.size(), 
		result.data(), result.size(), 
		0, 0
	);
	return result;
#endif
}
auto Avo::getNumberOfUnitsInUtfConvertedString(std::u16string_view const p_input) -> Count
{
#ifdef _WIN32
	return WideCharToMultiByte(
		CP_UTF8, 0, 
		reinterpret_cast<wchar_t const*>(p_input.data()), p_input.size(), 
		0, 0, 0, 0
	);
#endif
}

//------------------------------
// class Avo::Animation
//------------------------------

auto Avo::Animation::queueUpdate() -> void
{
	if (!m_isInUpdateQueue)
	{
		m_gui->m_animationUpdateQueue.emplace_back(this);
		m_isInUpdateQueue = true;
	}
}

auto Avo::Animation::cancelAllUpdates() -> void
{
	m_areUpdatesCancelled = true;

	// The animation might currently be in the animation update queue.
	auto& queue = m_gui->m_animationUpdateQueue;

	if (auto const position = std::find(queue.begin(), queue.end(), this);
		position != queue.end())
	{
		queue.erase(position);
	}
}

//------------------------------
// class Id
//------------------------------

Count Avo::Id::s_counter = {};

//------------------------------
// class Avo::View
//------------------------------

//
// Private
//

auto Avo::View::calculateAbsolutePositionRelativeTo(Avo::Point<> p_position) const -> Avo::Point<>
{
	auto container = getParent<View>();
	while (container && container != getGui())
	{
		p_position += container->getTopLeft();
		container = container->getParent<View>();
	}

	return p_position;
}

auto Avo::View::updateViewDrawingIndex(Avo::View* const p_view) -> void
{
	auto const numberOfViews = static_cast<Count>(m_childViews.size());
	if (numberOfViews <= 1 || p_view->getParent<View>() != this)
	{
		return;
	}

	auto const elevation = p_view->getElevation();
	if (!p_view->getIndex() || (p_view->getIndex() < numberOfViews - 1 && m_childViews[p_view->getIndex() + 1]->getElevation() < elevation))
	{
		for (auto a = p_view->getIndex(); a < numberOfViews; a++)
		{
			if (a == numberOfViews - 1 || m_childViews[a + 1]->getElevation() >= elevation)
			{
				m_childViews[a] = p_view;
				p_view->m_index = a;
				return;
			}
			else
			{
				m_childViews[a] = m_childViews[a + 1];
				m_childViews[a]->m_index = a;
			}
		}
	}
	else
	{
		for (auto a = p_view->getIndex(); a >= 0; a--)
		{
			if (!a || m_childViews[a - 1]->getElevation() <= elevation)
			{
				m_childViews[a] = p_view;
				p_view->m_index = a;
				return;
			}
			else
			{
				m_childViews[a] = m_childViews[a - 1];
				m_childViews[a]->m_index = a;
			}
		}
	}
}

auto Avo::View::updateShadow() -> void
{
	constexpr auto minElevation = 0.00001f;
	constexpr auto maxElevation = 400.f;
	if (getSize() >= 1.f && m_hasShadow && m_elevation > minElevation && m_elevation < maxElevation)
	{
		m_shadowImage = getDrawingContext()->createRectangleShadowImage(getSize(), m_corners, m_elevation, getThemeColor(ThemeColors::shadow));
		m_shadowBounds = {
			Avo::Point{
				0.5f*(m_bounds.right - m_bounds.left - m_shadowImage.getWidth()),
				0.35f*(m_bounds.bottom - m_bounds.top - m_shadowImage.getHeight())
			}, m_shadowImage.getSize()
		};
		m_shadowImage.setTopLeft(m_shadowBounds.getTopLeft());
	}
	else
	{
		m_shadowBounds = m_bounds.getSize();
	}
}

auto Avo::View::sendBoundsChangeEvents(Avo::Rectangle<> const p_previousBounds) -> void
{
	if (p_previousBounds != m_bounds)
	{
		if (auto const previousSize = p_previousBounds.getSize();
		    fabsf(previousSize.width - m_bounds.getWidth()) > 0.001f || 
			fabsf(previousSize.height - m_bounds.getHeight()) > 0.001f)
		{
			updateShadow(); // This is to update the shadow bounds and image.

			sizeChangeListeners(previousSize);

			updateClipGeometry();
		}

		if (this != getGui() && getParent<View>())
		{
			auto const mousePosition = getGui()->getWindow()->getMousePosition() - getParent<View>()->getAbsoluteTopLeft();
			if (getIsContaining(mousePosition) != p_previousBounds.getIsContaining(mousePosition))
			{
				auto event = Avo::MouseEvent{};
				event.xy = mousePosition + getParent<View>()->getAbsoluteTopLeft();
				getGui()->handleGlobalMouseMove(event);
			}
		}

		boundsChangeListeners(p_previousBounds);
	}
}

//
// Protected
//

auto Avo::View::updateClipGeometry() -> void
{
	if (getHasCornerStyles())
	{
		m_clipGeometry = getGui()->getDrawingContext()->createCornerRectangleGeometry(getSize(), m_corners);
	}
}

//
// Public
//

Avo::View::View(Avo::View* const p_parent, Avo::Rectangle<> const p_bounds) :
	Component{p_parent},
	ProtectedRectangle{p_bounds},
	m_shadowBounds{p_bounds}
{
	mouseDownListeners += Avo::bind(&View::handleMouseDown, this);
	mouseUpListeners += Avo::bind(&View::handleMouseUp, this);
	mouseScrollListeners += Avo::bind(&View::handleMouseScroll, this);
	mouseMoveListeners += Avo::bind(&View::handleMouseMove, this);
	mouseEnterListeners += Avo::bind(&View::handleMouseEnter, this);
	mouseLeaveListeners += Avo::bind(&View::handleMouseLeave, this);
	mouseBackgroundEnterListeners += Avo::bind(&View::handleMouseBackgroundEnter, this);
	mouseBackgroundLeaveListeners += Avo::bind(&View::handleMouseBackgroundLeave, this);

	dragDropEnterListeners += Avo::bind(&View::handleDragDropEnter, this);
	dragDropLeaveListeners += Avo::bind(&View::handleDragDropLeave, this);
	dragDropBackgroundEnterListeners += Avo::bind(&View::handleDragDropBackgroundEnter, this);
	dragDropBackgroundLeaveListeners += Avo::bind(&View::handleDragDropBackgroundLeave, this);
	dragDropMoveListeners += Avo::bind(&View::handleDragDropMove, this);
	dragDropFinishListeners += Avo::bind(&View::handleDragDropFinish, this);

	characterInputListeners += Avo::bind(&View::handleCharacterInput, this);
	keyboardKeyDownListeners += Avo::bind(&View::handleKeyboardKeyDown, this);
	keyboardKeyUpListeners += Avo::bind(&View::handleKeyboardKeyUp, this);
	keyboardFocusLoseListeners += Avo::bind(&View::handleKeyboardFocusLose, this);
	keyboardFocusGainListeners += Avo::bind(&View::handleKeyboardFocusGain, this);

	themeColorChangeListeners += Avo::bind(&View::handleThemeColorChange, this);
	themeEasingChangeListeners += Avo::bind(&View::handleThemeEasingChange, this);
	themeValueChangeListeners += Avo::bind(&View::handleThemeValueChange, this);

	boundsChangeListeners += Avo::bind(&View::handleBoundsChange, this);
	sizeChangeListeners += Avo::bind((void (View::*)(Avo::Size<>))&View::handleSizeChange, this);
	childViewAttachmentListeners += Avo::bind(&View::handleChildViewAttachment, this);
	childViewDetachmentListeners += Avo::bind(&View::handleChildViewDetachment, this);

	if (p_parent && p_parent != this)
	{
		setParent(p_parent);

		m_gui = m_parent->getGui();

		// No undefined behavior because getTheme converts Theme* to Theme const*.
		// We're just casting back to Theme*.
		m_theme = const_cast<Theme*>(m_parent->getTheme());
		m_theme->remember();
	}
	else
	{
		m_theme = new Theme;
	}
}
Avo::View::View(Avo::View* const p_parent, Avo::Id const p_id, Avo::Rectangle<> const p_bounds) :
	View{p_parent, p_bounds}
{
	setId(p_id, getGui());
}
Avo::View::~View()
{
	m_theme->forget();
	removeAllChildViews();
	if (m_parent)
	{
		remember();
		m_parent->removeChildView(m_index);
	}
}

//------------------------------

auto Avo::View::getDrawingContext() const -> Avo::DrawingContext*
{
	return m_gui->getDrawingContext();
}
auto Avo::View::getWindow() const -> Avo::Window*
{
	return m_gui->getWindow();
}

//------------------------------

auto Avo::View::setHasShadow(bool const p_hasShadow) -> void
{
	if (m_hasShadow != p_hasShadow)
	{
		m_hasShadow = p_hasShadow;
		if (m_hasShadow)
		{
			updateShadow();
		}
		else if (m_shadowImage)
		{
			m_shadowImage.destroy();
			m_shadowBounds = m_bounds.getSize();
		}
	}
}

//------------------------------

auto Avo::View::queueAnimationUpdate() -> void
{
	if (!m_isInAnimationUpdateQueue && m_gui && m_isVisible)
	{
		m_gui->m_viewAnimationUpdateQueue.push_back(this);
		remember();

		m_isInAnimationUpdateQueue = true;
	}
}

//------------------------------

auto Avo::View::handleMouseBackgroundEnter(MouseEvent const& p_event) -> void
{
	getGui()->getWindow()->setCursor(m_cursor);
}

//------------------------------

auto Avo::View::invalidate() -> void
{
	if (m_gui)
	{
		auto shadowBounds = getAbsoluteShadowBounds();

		auto dipToPixelFactor = m_gui->m_window->getDipToPixelFactor();
		shadowBounds.left = floor(shadowBounds.left*dipToPixelFactor)/dipToPixelFactor;
		shadowBounds.top = floor(shadowBounds.top*dipToPixelFactor)/dipToPixelFactor;
		shadowBounds.right = ceil(shadowBounds.right*dipToPixelFactor)/dipToPixelFactor;
		shadowBounds.bottom = ceil(shadowBounds.bottom*dipToPixelFactor)/dipToPixelFactor;

		if (shadowBounds == m_lastInvalidatedShadowBounds || 
			(!m_lastInvalidatedShadowBounds.getWidth() && !m_lastInvalidatedShadowBounds.getHeight()))
		{
			m_gui->invalidateRectangle(shadowBounds);
		}
		else if (shadowBounds.getIsIntersecting(m_lastInvalidatedShadowBounds))
		{
			m_gui->invalidateRectangle(m_lastInvalidatedShadowBounds.copy().contain(shadowBounds));
		}
		else
		{
			m_gui->invalidateRectangle(shadowBounds);
			m_gui->invalidateRectangle(m_lastInvalidatedShadowBounds);
		}

		m_lastInvalidatedShadowBounds = shadowBounds;
	}
}

auto Avo::View::drawShadow(DrawingContext* const p_drawingContext) -> void
{
	if (m_shadowImage && m_hasShadow)
	{
		p_drawingContext->setColor({1.f});
		p_drawingContext->drawImage(m_shadowImage, m_opacity);
	}
}

//------------------------------

#ifdef _WIN32

#define IUnknownDefinition(p_interfaceName)\
private:\
	ULONG m_referenceCount = 1ul;\
public:\
	auto __stdcall AddRef() -> ULONG override\
	{\
		return InterlockedIncrement(&m_referenceCount);\
	}\
	auto __stdcall Release() -> ULONG override\
	{\
		auto const referenceCount = InterlockedDecrement(&m_referenceCount);\
		if (!referenceCount)\
		{\
			delete this;\
			return 0;\
		}\
		return referenceCount;\
	}\
	auto __stdcall QueryInterface(IID const& p_id, void** const p_object) -> HRESULT override\
	{\
		if (p_id == IID_IUnknown || p_id == __uuidof(p_interfaceName))\
		{\
			*p_object = this;\
			AddRef();\
			return S_OK;\
		}\
		*p_object = 0;\
		return E_NOINTERFACE;\
	}

template<typename T, typename = std::enable_if_t<std::is_base_of_v<IUnknown, T>>>
class ComReference
{
	T* m_pointer = nullptr;
public:
	operator T* () const
	{
		return m_pointer;
	}
	auto operator&() -> T**
	{
		return &m_pointer;
	}
	auto operator->() const -> T*
	{
		return m_pointer;
	}
	auto operator*() const -> T*
	{
		return m_pointer;
	}
	/*
		Steals our reference to the pointer and returns the raw pointer.
		This object then doesn't reference anything anymore.
	*/
	[[nodiscard]] auto steal() -> T*
	{
		auto* const oldPointer = m_pointer;
		m_pointer = nullptr;
		return oldPointer;
	}
	template<typename _Target>
	[[nodiscard]] auto as() -> ComReference<_Target>
	{
		_Target* pointer;
		m_pointer->QueryInterface(&pointer);
		return pointer;
	}

	/*
		Adds a reference.
	*/
	auto operator=(T* const p_pointer) -> auto&
	{
		if (m_pointer)
		{
			m_pointer->Release();
		}
		m_pointer = p_pointer;
		if (m_pointer)
		{
			m_pointer->AddRef();
		}
		return *this;
	}
	auto operator=(ComReference<T>&& p_other) noexcept -> auto&
	{
		if (m_pointer)
		{
			m_pointer->Release();
		}
		m_pointer = p_other.m_pointer;
		p_other.m_pointer = nullptr;
		return *this;
	}
	auto operator=(ComReference<T> const& p_other) -> auto&
	{
		if (m_pointer)
		{
			m_pointer->Release();
		}
		m_pointer = p_other.m_pointer;
		if (m_pointer)
		{
			m_pointer->AddRef();
		}
		return *this;
	}

	/*
		Transfers ownership.
	*/
	ComReference(T* const p_pointer) :
		m_pointer{p_pointer}
	{
		// if (m_pointer)
		// {
		// 	m_pointer->AddRef();
		// }
	}
	ComReference(ComReference<T>&& p_other) noexcept :
		m_pointer{p_other.m_pointer}
	{
		p_other.m_pointer = nullptr;
	}
	ComReference(ComReference<T> const& p_other) :
		m_pointer{p_other.m_pointer}
	{
		if (m_pointer)
		{
			m_pointer->AddRef();
		}
	}
	ComReference() = default;
	~ComReference()
	{
		if (m_pointer)
		{
			m_pointer->Release();
		}
	}
};

template<typename T, typename ... Arguments>
auto makeComReference(Arguments&& ... p_arguments) -> ComReference<T>
{
	return ComReference<T>{new T{std::forward<Arguments>(p_arguments)...}};
}

//------------------------------

auto convertWindowsKeyStateToModifierKeyFlags(unsigned short const p_keyState) -> Avo::ModifierKeyFlags
{
	auto modifierFlags = Avo::ModifierKeyFlags::None;

	if (p_keyState & MK_CONTROL)
		modifierFlags |= Avo::ModifierKeyFlags::Control;
	if (p_keyState & MK_SHIFT)
		modifierFlags |= Avo::ModifierKeyFlags::Shift;
	if (p_keyState & MK_LBUTTON)
		modifierFlags |= Avo::ModifierKeyFlags::LeftMouse;
	if (p_keyState & MK_MBUTTON)
		modifierFlags |= Avo::ModifierKeyFlags::MiddleMouse;
	if (p_keyState & MK_RBUTTON)
		modifierFlags |= Avo::ModifierKeyFlags::RightMouse;
	if (p_keyState & MK_XBUTTON1)
		modifierFlags |= Avo::ModifierKeyFlags::X0Mouse;
	if (p_keyState & MK_XBUTTON2)
		modifierFlags |= Avo::ModifierKeyFlags::X1Mouse;
	if (GetKeyState(VK_MENU) < 0)
		modifierFlags |= Avo::ModifierKeyFlags::Alt;

	return modifierFlags;
}

//------------------------------

class OleFormatEnumerator : public IEnumFORMATETC
{
	IUnknownDefinition(IEnumFORMATETC)

private:
	std::vector<FORMATETC> m_formats;
	Index m_currentFormatIndex{};

public:
	auto __stdcall Next(ULONG p_numberOfFormatsToGet, FORMATETC* p_formats, ULONG* p_numberOfFormatsGotten) -> HRESULT override
	{
		auto numberOfFormatsGotten = Count{};
		while (m_currentFormatIndex < m_formats.size() && numberOfFormatsGotten < p_numberOfFormatsToGet)
		{
			*p_formats = m_formats[m_currentFormatIndex];

			m_currentFormatIndex++;
			numberOfFormatsGotten++;
			p_formats++;
		}
		if (p_numberOfFormatsGotten)
		{
			*p_numberOfFormatsGotten = numberOfFormatsGotten;
		}
		return p_numberOfFormatsToGet == numberOfFormatsGotten ? S_OK : S_FALSE;
	}

	auto __stdcall Skip(ULONG p_offset) -> HRESULT override
	{
		m_currentFormatIndex += p_offset;
		return m_currentFormatIndex < m_formats.size() ? S_OK : S_FALSE;
	}

	auto __stdcall Reset() -> HRESULT override
	{
		m_currentFormatIndex = {};
		return S_OK;
	}

	auto __stdcall Clone(IEnumFORMATETC** p_formatEnumerator) -> HRESULT override
	{
		auto newFormatEnumerator = new OleFormatEnumerator{m_formats};
		newFormatEnumerator->m_currentFormatIndex = m_currentFormatIndex;
		*p_formatEnumerator = newFormatEnumerator;
		return S_OK;
	}

	//------------------------------
	
	OleFormatEnumerator(Range<FORMATETC*> p_formats) :
		m_formats(p_formats.size())
	{
		for (auto a : Indices{p_formats})
		{
			m_formats[a] = p_formats[a];
			if (m_formats[a].ptd)
			{
				m_formats[a].ptd = static_cast<DVTARGETDEVICE*>(CoTaskMemAlloc(sizeof(DVTARGETDEVICE)));
				*m_formats[a].ptd = *p_formats[a].ptd;
			}
		}
	}
	~OleFormatEnumerator()
	{
		for (auto& format : m_formats)
		{
			if (format.ptd)
			{
				CoTaskMemFree(format.ptd);
			}
		}
	}
};

/*
	Communicates data in drag and drop operations.
*/
class OleDataObject : public IDataObject
{
	IUnknownDefinition(IDataObject)

private:
	std::vector<FORMATETC> m_formats;
	std::vector<STGMEDIUM> m_mediums;

public:
	auto __stdcall SetData(FORMATETC* p_format, STGMEDIUM* p_medium, BOOL p_willRelease) -> HRESULT override
	{
		if (p_willRelease)
		{
			bool hasFormat = false;
			for (auto a : Indices{m_formats})
			{
				if (m_formats[a].cfFormat == p_format->cfFormat &&
					m_formats[a].dwAspect == p_format->dwAspect &&
					m_formats[a].tymed & p_format->tymed &&
					m_formats[a].lindex == p_format->lindex)
				{
					hasFormat = true;
					ReleaseStgMedium(&m_mediums[a]);
					m_mediums[a] = *p_medium;
				}
			}
			if (!hasFormat)
			{
				m_formats.push_back(*p_format);
				m_mediums.push_back(*p_medium);
			}
			return S_OK;
		}
		else
		{
			return E_NOTIMPL;
		}
	}
	auto __stdcall QueryGetData(FORMATETC* p_format) -> HRESULT override
	{
		for (auto const& format : m_formats)
		{
			if (format.cfFormat == p_format->cfFormat &&
				format.dwAspect == p_format->dwAspect &&
				format.tymed & p_format->tymed && 
				format.lindex == p_format->lindex)
			{
				return S_OK;
			}
		}
		return DV_E_FORMATETC;
	}
	auto __stdcall GetData(FORMATETC* p_format, STGMEDIUM* p_medium) -> HRESULT override
	{
		for (auto a : Indices{m_formats})
		{
			if (m_formats[a].cfFormat == p_format->cfFormat &&
				m_formats[a].dwAspect == p_format->dwAspect &&
				m_formats[a].tymed & p_format->tymed &&
				m_formats[a].lindex == p_format->lindex)
			{
				p_medium->pUnkForRelease = {};
				if ((p_medium->tymed = m_formats[a].tymed) == TYMED_HGLOBAL)
				{
					// Copy memory from m_mediums[a].hGlobal to p_medium->hGlobal
					auto size = GlobalSize(m_mediums[a].hGlobal);
					p_medium->hGlobal = GlobalAlloc(GMEM_FIXED, size); // Returns pointer
					if (auto lockedPointer = GlobalLock(m_mediums[a].hGlobal); lockedPointer)
					{
						memcpy(p_medium->hGlobal, lockedPointer, size);
						GlobalUnlock(m_mediums[a].hGlobal);
					}
					else
					{
						// No need to unlock, because it was an unsuccessful lock.
						return E_FAIL;
					}
				}
				else if (p_medium->tymed == TYMED_ISTREAM)
				{
					p_medium->pstm = SHCreateMemStream(0, 0);

					auto stats = STATSTG{};
					m_mediums[a].pstm->Stat(&stats, STATFLAG_NONAME);
					p_medium->pstm->SetSize(stats.cbSize);

					m_mediums[a].pstm->Seek({0}, SEEK_SET, 0);
					m_mediums[a].pstm->CopyTo(p_medium->pstm, stats.cbSize, 0, 0);

					p_medium->pstm->Seek({0}, SEEK_SET, 0);
				}

				return S_OK;
			}
		}
		return DV_E_FORMATETC;
	}
	// Non-allocating version of GetData
	auto __stdcall GetDataHere(FORMATETC* p_format, STGMEDIUM* p_medium) -> HRESULT override
	{
		for (auto a : Indices{m_formats})
		{
			if (m_formats[a].cfFormat == p_format->cfFormat &&
				m_formats[a].dwAspect == p_format->dwAspect &&
				m_formats[a].tymed & p_format->tymed &&
				m_formats[a].lindex == p_format->lindex)
			{
				p_medium->pUnkForRelease = nullptr;
				if ((p_medium->tymed = m_formats[a].tymed) == TYMED_HGLOBAL)
				{
					// Copy memory from m_mediums[a].hGlobal to p_medium->hGlobal
					auto size = GlobalSize(m_mediums[a].hGlobal);
					memcpy(GlobalLock(p_medium->hGlobal), GlobalLock(m_mediums[a].hGlobal), size);
					GlobalUnlock(m_mediums[a].hGlobal);
					GlobalUnlock(p_medium->hGlobal);
				}
				else if (p_medium->tymed == TYMED_ISTREAM)
				{
					auto size = ULARGE_INTEGER{};
					size.QuadPart = ULONGLONG_MAX;
					m_mediums[a].pstm->CopyTo(p_medium->pstm, size, 0, 0);
				}

				return S_OK;
			}
		}
		return DV_E_FORMATETC;
	}

	auto __stdcall GetCanonicalFormatEtc(FORMATETC* p_formatIn, FORMATETC* p_formatOut) -> HRESULT override
	{
		p_formatOut->ptd = {};
		return E_NOTIMPL;
	}
	auto __stdcall EnumFormatEtc(DWORD p_direction, IEnumFORMATETC** p_formatEnumerator) -> HRESULT override
	{
		if (p_direction == DATADIR_GET)
		{
			*p_formatEnumerator = new OleFormatEnumerator{m_formats};
			return S_OK;
		}
		
		// it's DATADIR_SET
		return E_NOTIMPL;
	}

	auto __stdcall DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) -> HRESULT override
	{
		return OLE_E_ADVISENOTSUPPORTED;
	}
	auto __stdcall DUnadvise(DWORD) -> HRESULT override
	{
		return OLE_E_ADVISENOTSUPPORTED;
	}
	auto __stdcall EnumDAdvise(IEnumSTATDATA**) -> HRESULT override
	{
		return OLE_E_ADVISENOTSUPPORTED;
	}

	//------------------------------

	OleDataObject(Range<FORMATETC*> p_formats, Range<STGMEDIUM*> p_mediums)
	{
		m_formats.resize(p_formats.size());
		std::copy(p_formats.begin(), p_formats.end(), m_formats.begin());

		m_mediums.resize(p_mediums.size());
		std::copy(p_mediums.begin(), p_mediums.end(), m_mediums.begin());
	}
	__stdcall ~OleDataObject()
	{
		for (auto& medium : m_mediums)
		{
			ReleaseStgMedium(&medium);
		}
	}
};

//------------------------------

class OleDropSource : public IDropSource
{
	IUnknownDefinition(IDropSource)

private:
	Avo::Gui* m_gui;

	ComReference<IDragSourceHelper> m_dragImageHelper;

public:
	auto setDragImage(Avo::Image const& p_image, Avo::Point<> p_cursorPosition, IDataObject* p_dataObject) -> void
	{
		auto const size = p_image.getOriginalPixelSize();
		auto const dipToPixelFactor = m_gui->getWindow()->getDipToPixelFactor();

		SHDRAGIMAGE dragImage;
		dragImage.crColorKey = RGB(0, 0, 0);
		dragImage.hbmpDragImage = static_cast<HBITMAP>(m_gui->getDrawingContext()->createNativeImageFromImage(p_image));
		dragImage.ptOffset = {long(p_cursorPosition.x*dipToPixelFactor), long(p_cursorPosition.y*dipToPixelFactor)};
		dragImage.sizeDragImage = {(long)size.width, (long)size.height};

		m_dragImageHelper->InitializeFromBitmap(&dragImage, p_dataObject);
	}

	//------------------------------

	auto __stdcall QueryContinueDrag(BOOL p_wasEscapePressed, DWORD p_keyState) -> HRESULT override
	{
		if (p_wasEscapePressed)
		{
			return DRAGDROP_S_CANCEL;
		}
		if (!(p_keyState & MK_LBUTTON))
		{
			return DRAGDROP_S_DROP;
		}

		return S_OK;
	}

	auto __stdcall GiveFeedback(DWORD p_effect) -> HRESULT override
	{
		auto operation = Avo::DragDropOperation::None;
		switch (p_effect)
		{
			case DROPEFFECT_COPY:
				operation = Avo::DragDropOperation::Copy;
				break;
			case DROPEFFECT_MOVE:
				operation = Avo::DragDropOperation::Move;
				break;
			case DROPEFFECT_LINK:
				operation = Avo::DragDropOperation::Link;
		}
		m_gui->dragDropOperationChangeListeners(operation);
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	OleDropSource(Avo::Gui* p_gui) :
		m_gui{p_gui}
	{
		CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_dragImageHelper));
	}
};

//------------------------------

constexpr auto CLIPBOARD_FORMAT_ADDITIONAL_DATA = L"AvoGUI additional data";

class OleClipboardData : public Avo::ClipboardData
{
private:
	ComReference<IDataObject> m_dataObject;

	std::array<FORMATETC, 80> m_oleFormats{};
	Count m_numberOfFormats{};

	Count m_numberOfFiles{};

	FORMATETC* m_fileDescriptorFormat = nullptr;
	FORMATETC* m_itemNamesFormat = nullptr;
	FORMATETC* m_textFormat = nullptr;
	FORMATETC* m_additionalDataFormat = nullptr;

	uint32 m_clipboardFormat_fileContents;
	uint32 m_clipboardFormat_fileGroupDescriptor;
	uint32 m_clipboardFormat_additionalData;

	auto releaseDataObject() -> void
	{
		if (m_dataObject)
		{
			m_dataObject = nullptr;

			m_numberOfFormats = 0;
			m_numberOfFiles = 0;

			m_fileDescriptorFormat = nullptr;
			m_itemNamesFormat = nullptr;
			m_textFormat = nullptr;
			m_additionalDataFormat = nullptr;
		}
		for (auto& medium : m_globalDataToRelease)
		{
			GlobalUnlock(medium.hGlobal);
			ReleaseStgMedium(&medium);
		}
		m_streamBuffersToRelease.clear();
		m_globalDataToRelease.clear();
		formats.clear();
	}

public:
	auto setOleDataObject(IDataObject* const p_dataObject) -> void
	{
		releaseDataObject();
		if (m_dataObject = p_dataObject)
		{
			auto enumerator = ComReference<IEnumFORMATETC>{};
			m_dataObject->EnumFormatEtc(DATADIR_GET, &enumerator);

			auto numberOfFormats = ULONG{};
			enumerator->Next(80, m_oleFormats.data(), &numberOfFormats);
			m_numberOfFormats = numberOfFormats;

			for (auto a : Indices{m_numberOfFormats})
			{
				auto* format = &m_oleFormats[a];
				auto const formatId = m_oleFormats[a].cfFormat;
				if (formatId == CF_HDROP && format->dwAspect == DVASPECT_CONTENT)
				{
					m_itemNamesFormat = format;
				}
				else if (formatId == m_clipboardFormat_fileGroupDescriptor)
				{
					m_fileDescriptorFormat = format;
				}
				else if (formatId == m_clipboardFormat_fileContents)
				{
					m_numberOfFiles++;
				}
				else if (formatId == CF_UNICODETEXT)
				{
					m_textFormat = format;
				}
				else if (formatId == m_clipboardFormat_additionalData)
				{
					m_additionalDataFormat = format;
				}
				formats.push_back(format->cfFormat);
			}
		}
	}
	auto getOleDataObject() -> IDataObject*
	{
		return m_dataObject;
	}

private:
	std::vector<STGMEDIUM> mutable m_globalDataToRelease;
	std::vector<Avo::DataVector> mutable m_streamBuffersToRelease;
public:
	auto getDataForFormat(Index p_formatIndex) const -> Range<std::byte const*> override
	{
		switch (m_oleFormats[p_formatIndex].tymed)
		{
			case TYMED_HGLOBAL:
			{
				// GetData does not mutate m_oleFormats.
				if (STGMEDIUM medium; m_dataObject->GetData(const_cast<FORMATETC*>(&m_oleFormats[p_formatIndex]), &medium) == S_OK)
				{
					m_globalDataToRelease.push_back(medium);
					return {static_cast<std::byte const*>(GlobalLock(medium.hGlobal)), static_cast<Count>(GlobalSize(medium.hGlobal))};
				}
				break;
			}
			case TYMED_ISTREAM:
			{
				// GetData does not mutate m_oleFormats.
				if (STGMEDIUM medium; m_dataObject->GetData(const_cast<FORMATETC*>(&m_oleFormats[p_formatIndex]), &medium) == S_OK)
				{
					STATSTG stats;
					medium.pstm->Stat(&stats, STATFLAG_NONAME);

					auto& buffer = m_streamBuffersToRelease.emplace_back(stats.cbSize.QuadPart);
					auto numberOfBytesRead = ULONG{};
					medium.pstm->Seek({0}, SEEK_SET, 0);
					medium.pstm->Read(static_cast<void*>(buffer.data()), stats.cbSize.QuadPart, &numberOfBytesRead);
					ReleaseStgMedium(&medium);

					return {buffer.data(), numberOfBytesRead};
				}
				break;
			}
		}
		return {nullptr, 0};
	}
	auto getFormatName(uint32 p_format) const -> std::string override
	{
		wchar_t name[51];
		auto const length = GetClipboardFormatNameW(p_format, name, 50);
		if (!length)
		{
			switch (p_format)
			{
				case CF_BITMAP:
					return "CF_BITMAP";
				case CF_DIB:
					return "CF_DIB";
				case CF_DIF:
					return "CF_DIF";
				case CF_DSPBITMAP:
					return "CF_DSPBITMAP";
				case CF_DSPENHMETAFILE:
					return "CF_DSPENHMETAFILE";
				case CF_DSPMETAFILEPICT:
					return "CF_DSPMETAFILEPICT";
				case CF_DSPTEXT:
					return "CF_DSPTEXT";
				case CF_ENHMETAFILE:
					return "CF_ENHMETAFILE";
				case CF_GDIOBJFIRST:
					return "CF_GDIOBJFIRST";
				case CF_GDIOBJLAST:
					return "CF_GDIOBJLAST";
				case CF_HDROP:
					return "CF_HDROP";
				case CF_LOCALE:
					return "CF_LOCALE";
				case CF_METAFILEPICT:
					return "CF_METAFILEPICT";
				case CF_OEMTEXT:
					return "CF_OEMTEXT";
				case CF_OWNERDISPLAY:
					return "CF_OWNERDISPLAY";
				case CF_PALETTE:
					return "CF_PALETTE";
				case CF_PENDATA:
					return "CF_PENDATA";
				case CF_PRIVATEFIRST:
					return "CF_PRIVATEFIRST";
				case CF_PRIVATELAST:
					return "CF_PRIVATELAST";
				case CF_RIFF:
					return "CF_RIFF";
				case CF_SYLK:
					return "CF_SYLK";
				case CF_TEXT:
					return "CF_TEXT";
				case CF_TIFF:
					return "CF_TIFF";
				case CF_UNICODETEXT:
					return "CF_UNICODETEXT";
				case CF_WAVE:
					return "CF_WAVE";
			}
			return "Unknown";
		}
		name[50] = 0;
		return Avo::convertUtf16ToUtf8(reinterpret_cast<char16 const*>(name));
	}

private:
	template<typename StringType>
	auto getString() const -> StringType
	{
		constexpr bool isUtf8 = std::is_same_v<StringType, std::string>;

		if (m_textFormat)
		{
			if (STGMEDIUM medium; m_dataObject->GetData(m_textFormat, &medium) == S_OK)
			{
				Avo::Cleanup cleanup = [&] {
					GlobalUnlock(medium.hGlobal);
					ReleaseStgMedium(&medium);
				};
				if constexpr (isUtf8)
				{
					return Avo::convertUtf16ToUtf8(static_cast<char16 const*>(GlobalLock(medium.hGlobal)));
				}
				else
				{
					return static_cast<char16 const*>(GlobalLock(medium.hGlobal));
				}
			}
		}
		
		if constexpr (isUtf8)
		{
			return u8"";
		}
		else
		{
			return u"";
		}
	}

public:
	auto getString() const -> std::string override
	{
		return getString<std::string>();
	}
	auto getUtf16String() const -> std::u16string override
	{
		return getString<std::u16string>();
	}
	auto getHasString() const -> bool override
	{
		return m_textFormat;
	}

private:
	template<typename StringType>
	auto getItemNames() const -> std::vector<StringType>
	{
		if (m_itemNamesFormat)
		{
			std::vector<StringType> itemNames;

			if (auto medium = STGMEDIUM{}; m_dataObject->GetData(m_itemNamesFormat, &medium) == S_OK)
			{
				Avo::Cleanup const cleanup_releaseMedium = [&] { ReleaseStgMedium(&medium); };

				if (medium.tymed == TYMED_HGLOBAL)
				{
					auto const filenameStructure = static_cast<DROPFILES const*>(GlobalLock(medium.hGlobal));
					Avo::Cleanup const globalMemoryLockGuard = [&] { GlobalUnlock(medium.hGlobal); };

					// When you have to deal with C in C++...
					auto currentBufferPosition = reinterpret_cast<char16 const*>(
						reinterpret_cast<char const*>(filenameStructure) + filenameStructure->pFiles
					);
					while (*currentBufferPosition)
					{
						if constexpr (std::is_same_v<StringType, std::string>)
						{
							currentBufferPosition += itemNames.emplace_back(Avo::convertUtf16ToUtf8(currentBufferPosition)).size() + 1;
						}
						else
						{
							currentBufferPosition += itemNames.emplace_back(currentBufferPosition).size() + 1;
						}
					}
				}
			}
			return itemNames;
		}
		return getFileNames<StringType>();
	}

public:
	auto getItemNames() const -> std::vector<std::string> override
	{
		return getItemNames<std::string>();
	}
	auto getUtf16ItemNames() const -> std::vector<std::u16string> override
	{
		return getItemNames<std::u16string>();
	}
	auto getNumberOfItemNames() const -> Count override
	{
		if (m_itemNamesFormat)
		{
			auto numberOfItemNames = Count{};

			if (auto medium = STGMEDIUM{}; m_dataObject->GetData(m_itemNamesFormat, &medium) == S_OK )
			{
				Avo::Cleanup const cleanup_releaseMedium = [&] { ReleaseStgMedium(&medium); };

				if (medium.tymed == TYMED_HGLOBAL)
				{
					auto filenameStructure = static_cast<DROPFILES const*>(GlobalLock(medium.hGlobal));
					Avo::Cleanup const globalMemoryLockGuard = [&] { GlobalUnlock(medium.hGlobal); };

					auto currentBufferPosition = reinterpret_cast<wchar_t const*>(
						reinterpret_cast<char const*>(filenameStructure) + filenameStructure->pFiles
					);
					while (*currentBufferPosition)
					{
						currentBufferPosition += std::wcslen(currentBufferPosition) + 1;
					}
				}
			}
			return numberOfItemNames;
		}
		return getNumberOfFiles();
	}

private:
	template<typename StringType>
	auto getFileNames() const -> std::vector<StringType>
	{
		if (m_fileDescriptorFormat)
		{
			auto fileNames = std::vector<StringType>();

			if (auto medium = STGMEDIUM{}; m_dataObject->GetData(m_fileDescriptorFormat, &medium) == S_OK)
			{
				Avo::Cleanup const cleanup_releaseMedium = [&]{ ReleaseStgMedium(&medium); };

				if (medium.tymed == TYMED_HGLOBAL)
				{
					auto const groupDescriptor = static_cast<FILEGROUPDESCRIPTORW*>(GlobalLock(medium.hGlobal));
					Avo::Cleanup const cleanup_unlockGlobalMemory = [&]{ GlobalUnlock(medium.hGlobal); };
					
					fileNames.reserve(groupDescriptor->cItems);
					for (auto const a : Indices{fileNames})
					{
						auto const fileNameWideString = reinterpret_cast<char16 const*>(groupDescriptor->fgd[a].cFileName);
						if constexpr (std::is_same_v<StringType, std::string>)
						{
							fileNames[a] = Avo::convertUtf16ToUtf8(fileNameWideString);
						}
						else
						{
							fileNames[a] = fileNameWideString;
						}
					}
				}
			}
			return fileNames;
		}
		return {};
	}

public:
	auto getFileNames() const -> std::vector<std::string> override
	{
		return getFileNames<std::string>();
	}
	auto getUtf16FileNames() const -> std::vector<std::u16string> override
	{
		return getFileNames<std::u16string>();
	}

	auto getFileContents() const -> std::vector<Avo::DataVector> override
	{
		if (m_fileDescriptorFormat)
		{
			auto output = std::vector<Avo::DataVector>();

			for (auto a : Indices{m_numberOfFormats})
			{
				if (m_oleFormats[a].cfFormat == m_clipboardFormat_fileContents)
				{
					// GetData shouldn't mutate m_oleFormats.
					if (auto medium = STGMEDIUM{}; 
					    m_dataObject->GetData(const_cast<FORMATETC*>(&m_oleFormats[a]), &medium) == S_OK)
					{
						Avo::Cleanup cleanup_releaseMedium = [&]{ ReleaseStgMedium(&medium); };
						
						if (medium.tymed == TYMED_HGLOBAL)
						{
							auto* const pointer = static_cast<std::byte*>(GlobalLock(medium.hGlobal));
							output.emplace_back(pointer, pointer + GlobalSize(medium.hGlobal));
							GlobalUnlock(medium.hGlobal);
						}
						else if (medium.tymed == TYMED_ISTREAM)
						{
							auto stats = STATSTG{};
							medium.pstm->Stat(&stats, STATFLAG_NONAME);

							output.emplace_back(stats.cbSize.QuadPart);

							auto bufferSize = ULONG{};
							medium.pstm->Seek({0}, SEEK_SET, 0);
							medium.pstm->Read(output.back().data(), output.size(), &bufferSize);
							if (bufferSize != output.back().size())
							{
								output.back().resize(bufferSize);
							}
						}
					}
				}
			}
			return output;
		}
		return {};
	}
	auto getFileContents(Index p_index) const -> Avo::DataVector override
	{
		if (m_fileDescriptorFormat)
		{
			auto currentIndex = Index{};
			for (auto const a : Indices{m_numberOfFormats})
			{
				if (m_oleFormats[a].cfFormat == m_clipboardFormat_fileContents)
				{
					if (currentIndex++ == p_index)
					{
						// GetData shouldn't mutate.
						if (auto medium = STGMEDIUM{}; m_dataObject->GetData(const_cast<FORMATETC*>(&m_oleFormats[a]), &medium) == S_OK)
						{
							Avo::Cleanup const cleanup_releaseMedium = [&]{ ReleaseStgMedium(&medium); };

							if (medium.tymed == TYMED_HGLOBAL)
							{
								auto* const pointer = static_cast<std::byte*>(GlobalLock(medium.hGlobal));
								auto result = Avo::DataVector(pointer, pointer + GlobalSize(medium.hGlobal));
								GlobalUnlock(medium.hGlobal);
								return result;
							}
							else if (medium.tymed == TYMED_ISTREAM)
							{
								auto stats = STATSTG{};
								medium.pstm->Stat(&stats, STATFLAG_NONAME);

								auto result = Avo::DataVector(stats.cbSize.QuadPart);

								auto bufferSize = ULONG{};
								medium.pstm->Seek({0}, SEEK_SET, 0);
								medium.pstm->Read(result.data(), result.size(), &bufferSize);
								if (bufferSize != result.size())
								{
									result.resize(bufferSize);
								}
								
								return result;
							}
						}
					}
				}
			}
		}

		return {};
	}
	auto getNumberOfFiles() const -> Count override
	{
		return m_numberOfFiles;
	}

	auto getAdditionalData() const -> uint64 override
	{
		if (!m_additionalDataFormat)
		{
			return 0u;
		}
		
		if (auto medium = STGMEDIUM{}; m_dataObject->GetData(m_additionalDataFormat, &medium) == S_OK)
		{
			return *static_cast<uint64*>(GlobalLock(medium.hGlobal)); GlobalUnlock(medium.hGlobal);
		}
		return 0u;
	}

	auto getImage() const -> Avo::Image override
	{
		if (m_numberOfFiles)
		{
			auto const file = getFileContents(0);
			return m_gui->getDrawingContext()->createImage({file.data(), static_cast<Count>(file.size())});
		}
		return {};
	}

private:
	Avo::Gui* m_gui;
public:
	OleClipboardData(Avo::Gui* p_gui) :
		m_gui{p_gui}
	{
		m_clipboardFormat_fileContents = RegisterClipboardFormatW(CFSTR_FILECONTENTS);
		m_clipboardFormat_fileGroupDescriptor = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
		m_clipboardFormat_additionalData = RegisterClipboardFormatW(CLIPBOARD_FORMAT_ADDITIONAL_DATA);
	}
	~OleClipboardData()
	{
		releaseDataObject();
	}
};

//------------------------------

class OleDropTarget : public IDropTarget
{
	IUnknownDefinition(IDropTarget)

private:	
	DWORD m_currentEffect;
	auto updateEffect(DWORD* const p_effect) -> void
	{
		switch (m_gui->getGlobalDragDropOperation(m_dragDropEvent))
		{
			case Avo::DragDropOperation::Copy:
				*p_effect = DROPEFFECT_COPY;
				break;
			case Avo::DragDropOperation::Move:
				*p_effect = DROPEFFECT_MOVE;
				break;
			case Avo::DragDropOperation::Link:
				*p_effect = DROPEFFECT_LINK;
				break;
			default:
				*p_effect = DROPEFFECT_NONE;
		}
		m_currentEffect = *p_effect;
	}

	Avo::DragDropEvent m_dragDropEvent;
	auto updateEventFromPoint(POINT& p_point) -> void
	{
		ScreenToClient(static_cast<HWND>(m_gui->getWindow()->getNativeHandle()), &p_point);

		auto const previousPosition = m_dragDropEvent.xy;
		m_dragDropEvent.xy = m_gui->getWindow()->pixelsToDips(Avo::Point<Avo::Pixels>{p_point.x, p_point.y});
		m_dragDropEvent.movement = Avo::Vector2d{m_dragDropEvent.xy - previousPosition};
	}

public:
	auto __stdcall DragEnter(
		IDataObject* const p_dataObject, 
		DWORD const p_keyState, 
		POINTL const p_mousePosition, 
		DWORD* const p_effect
	) -> HRESULT
	{
		m_dropData.setOleDataObject(p_dataObject);

		auto clientMousePosition = POINT{p_mousePosition.x, p_mousePosition.y};
		m_dropImageViewer->DragEnter(m_gui->getWindow()->getNativeHandle<HWND>(), p_dataObject, &clientMousePosition, *p_effect);
		updateEventFromPoint(clientMousePosition);

		m_dragDropEvent.modifierKeys = convertWindowsKeyStateToModifierKeyFlags(p_keyState);

		auto const lock = m_gui->createThreadLock();
		m_gui->handleGlobalDragDropEnter(m_dragDropEvent);
		updateEffect(p_effect);

		return S_OK;
	}
	auto __stdcall DragOver(
		DWORD const p_keyState, 
		POINTL const p_mousePosition, 
		DWORD* const p_effect
	) -> HRESULT
	{
		auto clientMousePosition = POINT{p_mousePosition.x, p_mousePosition.y};
		m_dropImageViewer->DragOver(&clientMousePosition, *p_effect);
		updateEventFromPoint(clientMousePosition);

		m_dragDropEvent.modifierKeys = convertWindowsKeyStateToModifierKeyFlags(p_keyState);

		auto const lock = m_gui->createThreadLock();
		m_gui->handleGlobalDragDropMove(m_dragDropEvent);
		updateEffect(p_effect);

		return S_OK;
	}
	auto __stdcall DragLeave() -> HRESULT
	{
		m_dropImageViewer->DragLeave();

		if (auto clientMousePosition = POINT{}; GetCursorPos(&clientMousePosition))
		{
			updateEventFromPoint(clientMousePosition);
		}		

		m_gui->lockThreads();
		m_gui->handleGlobalDragDropLeave(m_dragDropEvent);
		m_gui->unlockThreads();
		
		m_dropData.setOleDataObject(nullptr);

		return S_OK;
	}
	auto __stdcall Drop(
		IDataObject* const p_dataObject, 
		DWORD const p_keyState, 
		POINTL const p_mousePosition, 
		DWORD* const p_effect
	) -> HRESULT
	{
		auto clientMousePosition = POINT{p_mousePosition.x, p_mousePosition.y};
		updateEventFromPoint(clientMousePosition);

		m_dragDropEvent.modifierKeys = convertWindowsKeyStateToModifierKeyFlags(p_keyState);

		m_gui->lockThreads();
		m_gui->handleGlobalDragDropFinish(m_dragDropEvent);
		m_gui->unlockThreads();

		clientMousePosition = {p_mousePosition.x, p_mousePosition.y};
		m_dropImageViewer->Drop(p_dataObject, &clientMousePosition, *p_effect);

		m_dropData.setOleDataObject(nullptr);

		return S_OK;
	}

	//------------------------------

private:
	Avo::Gui* m_gui;
	OleClipboardData m_dropData;
	ComReference<IDropTargetHelper> m_dropImageViewer;

public:
	OleDropTarget(Avo::Gui* const p_gui) :
		m_gui{p_gui},
		m_dropData{p_gui}
	{
		m_dropData.formats.reserve(15);
		m_dragDropEvent.data = &m_dropData;

		CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_dropImageViewer));
	}
};

//------------------------------

constexpr int WM_APP_CHANGE_SIZE = WM_APP;
constexpr int WM_APP_SET_IS_ENABLED = WM_APP + 1;

class WindowsWindow : public Avo::Window
{
public:
	static std::atomic<uint32> s_numberOfWindows;

private:
	HWND m_windowHandle = nullptr;
	Avo::WindowStyleFlags m_crossPlatformStyles;
	uint32 m_styles{};

	auto getHasCustomBorder() -> bool
	{
		return static_cast<bool>(m_crossPlatformStyles & Avo::WindowStyleFlags::CustomBorder);
	}
	auto convertWindowStyleFlagsToWindowsWindowStyleFlags(
		Avo::WindowStyleFlags const p_styleFlags, 
		bool const p_hasParent
	) -> uint32
	{
		auto styles = uint32{};

		if (p_hasParent || static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::CustomBorder))
		{
			styles |= WS_POPUP;
		}

		if (!static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::Invisible))
			styles |= WS_VISIBLE;
		if (static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::CloseButton) || 
			static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::CustomBorder))
			styles |= WS_CAPTION | WS_SYSMENU;

		if (static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::Minimized))
			styles |= WS_MINIMIZE;
		else if (static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::Maximized))
			styles |= WS_MAXIMIZE;

		if (static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::MinimizeButton))
			styles |= WS_MINIMIZEBOX;
		if (static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::MaximizeButton))
			styles |= WS_MAXIMIZEBOX;
		if (static_cast<bool>(p_styleFlags & Avo::WindowStyleFlags::Resizable))
			styles |= WS_THICKFRAME;

		return styles;
	}
	auto convertWindowsDataToKeyboardKey(uint64 const p_data) -> Avo::KeyboardKey
	{
		switch (p_data)
		{
			case VK_APPS:
				return Avo::KeyboardKey::Menu;
			case VK_BACK:
				return Avo::KeyboardKey::Backspace;
			case VK_CLEAR:
				return Avo::KeyboardKey::Clear;
			case VK_TAB:
				return Avo::KeyboardKey::Tab;
			case VK_RETURN:
				return Avo::KeyboardKey::Return;
			case VK_SHIFT:
				return Avo::KeyboardKey::Shift;
			case VK_CONTROL:
				return Avo::KeyboardKey::Control;
			case VK_MENU:
				return Avo::KeyboardKey::Alt;
			case VK_PAUSE:
				return Avo::KeyboardKey::Pause;
			case VK_CAPITAL:
				return Avo::KeyboardKey::CapsLock;
			case VK_ESCAPE:
				return Avo::KeyboardKey::Escape;
			case VK_SPACE:
				return Avo::KeyboardKey::Spacebar;
			case VK_PRIOR:
				return Avo::KeyboardKey::PageUp;
			case VK_NEXT:
				return Avo::KeyboardKey::PageDown;
			case VK_END:
				return Avo::KeyboardKey::End;
			case VK_HOME:
				return Avo::KeyboardKey::Home;
			case VK_LEFT:
				return Avo::KeyboardKey::Left;
			case VK_RIGHT:
				return Avo::KeyboardKey::Right;
			case VK_UP:
				return Avo::KeyboardKey::Up;
			case VK_DOWN:
				return Avo::KeyboardKey::Down;
			case VK_SNAPSHOT:
				return Avo::KeyboardKey::PrintScreen;
			case VK_INSERT:
				return Avo::KeyboardKey::Insert;
			case VK_DELETE:
				return Avo::KeyboardKey::Delete;
			case VK_HELP:
				return Avo::KeyboardKey::Help;
			case VK_NUMPAD0:
				return Avo::KeyboardKey::Numpad0;
			case VK_NUMPAD1:
				return Avo::KeyboardKey::Numpad1;
			case VK_NUMPAD2:
				return Avo::KeyboardKey::Numpad2;
			case VK_NUMPAD3:
				return Avo::KeyboardKey::Numpad3;
			case VK_NUMPAD4:
				return Avo::KeyboardKey::Numpad4;
			case VK_NUMPAD5:
				return Avo::KeyboardKey::Numpad5;
			case VK_NUMPAD6:
				return Avo::KeyboardKey::Numpad6;
			case VK_NUMPAD7:
				return Avo::KeyboardKey::Numpad7;
			case VK_NUMPAD8:
				return Avo::KeyboardKey::Numpad8;
			case VK_NUMPAD9:
				return Avo::KeyboardKey::Numpad9;
			case VK_ADD:
				return Avo::KeyboardKey::Add;
			case VK_SUBTRACT:
				return Avo::KeyboardKey::Subtract;
			case VK_MULTIPLY:
				return Avo::KeyboardKey::Multiply;
			case VK_DIVIDE:
				return Avo::KeyboardKey::Divide;
			case VK_F1:
				return Avo::KeyboardKey::F1;
			case VK_F2:
				return Avo::KeyboardKey::F2;
			case VK_F3:
				return Avo::KeyboardKey::F3;
			case VK_F4:
				return Avo::KeyboardKey::F4;
			case VK_F5:
				return Avo::KeyboardKey::F5;
			case VK_F6:
				return Avo::KeyboardKey::F6;
			case VK_F7:
				return Avo::KeyboardKey::F7;
			case VK_F8:
				return Avo::KeyboardKey::F8;
			case VK_F9:
				return Avo::KeyboardKey::F9;
			case VK_F10:
				return Avo::KeyboardKey::F10;
			case VK_F11:
				return Avo::KeyboardKey::F11;
			case VK_F12:
				return Avo::KeyboardKey::F12;
			case VK_F13:
				return Avo::KeyboardKey::F13;
			case VK_F14:
				return Avo::KeyboardKey::F14;
			case VK_F15:
				return Avo::KeyboardKey::F15;
			case VK_F16:
				return Avo::KeyboardKey::F16;
			case VK_F17:
				return Avo::KeyboardKey::F17;
			case VK_F18:
				return Avo::KeyboardKey::F18;
			case VK_F19:
				return Avo::KeyboardKey::F19;
			case VK_F20:
				return Avo::KeyboardKey::F20;
			case VK_F21:
				return Avo::KeyboardKey::F21;
			case VK_F22:
				return Avo::KeyboardKey::F22;
			case VK_F23:
				return Avo::KeyboardKey::F23;
			case VK_F24:
				return Avo::KeyboardKey::F24;
			case VK_NUMLOCK:
				return Avo::KeyboardKey::NumLock;
			case 0x30:
				return Avo::KeyboardKey::Number0;
			case 0x31:
				return Avo::KeyboardKey::Number1;
			case 0x32:
				return Avo::KeyboardKey::Number2;
			case 0x33:
				return Avo::KeyboardKey::Number3;
			case 0x34:
				return Avo::KeyboardKey::Number4;
			case 0x35:
				return Avo::KeyboardKey::Number5;
			case 0x36:
				return Avo::KeyboardKey::Number6;
			case 0x37:
				return Avo::KeyboardKey::Number7;
			case 0x38:
				return Avo::KeyboardKey::Number8;
			case 0x39:
				return Avo::KeyboardKey::Number9;
			case 0x41:
				return Avo::KeyboardKey::A;
			case 0x42:
				return Avo::KeyboardKey::B;
			case 0x43:
				return Avo::KeyboardKey::C;
			case 0x44:
				return Avo::KeyboardKey::D;
			case 0x45:
				return Avo::KeyboardKey::E;
			case 0x46:
				return Avo::KeyboardKey::F;
			case 0x47:
				return Avo::KeyboardKey::G;
			case 0x48:
				return Avo::KeyboardKey::H;
			case 0x49:
				return Avo::KeyboardKey::I;
			case 0x4A:
				return Avo::KeyboardKey::J;
			case 0x4B:
				return Avo::KeyboardKey::K;
			case 0x4C:
				return Avo::KeyboardKey::L;
			case 0x4D:
				return Avo::KeyboardKey::M;
			case 0x4E:
				return Avo::KeyboardKey::N;
			case 0x4F:
				return Avo::KeyboardKey::O;
			case 0x50:
				return Avo::KeyboardKey::P;
			case 0x51:
				return Avo::KeyboardKey::Q;
			case 0x52:
				return Avo::KeyboardKey::R;
			case 0x53:
				return Avo::KeyboardKey::S;
			case 0x54:
				return Avo::KeyboardKey::T;
			case 0x55:
				return Avo::KeyboardKey::U;
			case 0x56:
				return Avo::KeyboardKey::V;
			case 0x57:
				return Avo::KeyboardKey::W;
			case 0x58:
				return Avo::KeyboardKey::X;
			case 0x59:
				return Avo::KeyboardKey::Y;
			case 0x5A:
				return Avo::KeyboardKey::Z;
			case VK_OEM_COMMA:
				return Avo::KeyboardKey::Comma;
			case VK_OEM_PERIOD:
				return Avo::KeyboardKey::Period;
			case VK_OEM_PLUS:
				return Avo::KeyboardKey::Plus;
			case VK_OEM_MINUS:
				return Avo::KeyboardKey::Minus;
			case VK_OEM_1:
				return Avo::KeyboardKey::Regional1;
			case VK_OEM_2:
				return Avo::KeyboardKey::Regional2;
			case VK_OEM_3:
				return Avo::KeyboardKey::Regional3;
			case VK_OEM_4:
				return Avo::KeyboardKey::Regional4;
			case VK_OEM_5:
				return Avo::KeyboardKey::Regional5;
			case VK_OEM_6:
				return Avo::KeyboardKey::Regional6;
			case VK_OEM_7:
				return Avo::KeyboardKey::Regional7;
		}
		return Avo::KeyboardKey::None;
	}

	//------------------------------

	inline static auto const WINDOW_CLASS_NAME = L"AvoGUI Window";

	bool m_hasCreatedWindow = false;
	std::condition_variable m_hasCreatedWindowConditionVariable;
	std::mutex m_hasCreatedWindowMutex;
	std::thread m_messageThread;
	auto thread_createAndRun(
		std::string_view const p_title, 
	    Avo::Point<> const p_position, 
		Avo::Size<> const p_size, 
	    Avo::Window* const p_parent
	) -> void
	{
		if (m_windowHandle)
		{
			DestroyWindow(m_windowHandle);
			s_numberOfWindows--;
		}
		else if (!s_numberOfWindows)
		{
			auto const windowClass = WNDCLASSW{
				/*.style = */CS_DBLCLKS,
				/*.lpfnWndProc = */handleGlobalEvents,
				0, 0,
				/*.hInstance = */GetModuleHandle(0),
				/*.hIcon = */nullptr,
				/*.hCursor = */nullptr,
				/*.hbrBackground = */nullptr,
				/*.lpszMenuName = */nullptr,
				/*.lpszClassName = */WINDOW_CLASS_NAME,
			};
			RegisterClassW(&windowClass);
		}

		//------------------------------

		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		m_dipToPixelFactor = GetDpiForSystem()/(float)USER_DEFAULT_SCREEN_DPI;

		// Calculate nonclient window rectangle from client size.
		auto windowRect = RECT{0, 0, (int)std::ceil(p_size.x*m_dipToPixelFactor), (int)std::ceil(p_size.y*m_dipToPixelFactor)};
		m_size = {windowRect.right, windowRect.bottom}; // Client area
		
		AdjustWindowRect(&windowRect, m_styles, 0);

		//------------------------------

		auto cursorPosition = POINT{};
		GetCursorPos(&cursorPosition);
		m_mousePosition = {cursorPosition.x, cursorPosition.y};

		auto parentRect = RECT{};

		if (p_parent)
		{
			GetWindowRect(static_cast<HWND>(p_parent->getNativeHandle()), &parentRect);
		}
		else
		{
			auto const monitor = MonitorFromPoint(cursorPosition, MONITOR_DEFAULTTONEAREST);
			auto monitorInfo = MONITORINFO{};
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &monitorInfo);
			parentRect = monitorInfo.rcWork;
		}

		//------------------------------

		auto const wideTitle = Avo::convertUtf8ToUtf16(p_title);

		// m_windowHandle is initialized by the WM_CREATE event, before CreateWindow returns.

		auto const parentWindowHandle = p_parent ? static_cast<HWND>(p_parent->getNativeHandle()) : nullptr;

		if (getHasCustomBorder())
		{
			CreateWindowW(
				WINDOW_CLASS_NAME,
				reinterpret_cast<LPCWSTR>(wideTitle.c_str()),
				m_styles,
				0, 0,
				0, 0,
				parentWindowHandle,
				0, // No menu
				GetModuleHandle(0),
				this // Additional window data - the instance
			);

			/*
				Remove border, can't do that just with styles without losing the window shadow.
			*/
			auto const margins = MARGINS{0, 0, 0, 1};
			DwmExtendFrameIntoClientArea(m_windowHandle, &margins);

			SetWindowPos(
				m_windowHandle, nullptr,
				parentRect.left + windowRect.left + p_position.x*(parentRect.right - parentRect.left - m_size.x),
				parentRect.top + windowRect.top + p_position.y*(parentRect.bottom - parentRect.top - m_size.y),
				m_size.x,
				m_size.y,
				SWP_FRAMECHANGED | SWP_NOZORDER
			);
		}
		else
		{
			CreateWindowW(
				WINDOW_CLASS_NAME,
				reinterpret_cast<LPCWSTR>(wideTitle.c_str()),
				m_styles,
				parentRect.left + windowRect.left + p_position.x*(parentRect.right - parentRect.left - windowRect.right + windowRect.left),
				parentRect.top + windowRect.top + p_position.y*(parentRect.bottom - parentRect.top - windowRect.bottom + windowRect.top),
				windowRect.right - windowRect.left,
				windowRect.bottom - windowRect.top,
				parentWindowHandle,
				0, // No menu
				GetModuleHandle(0),
				this // Additional window data - the instance
			);
		}
		s_numberOfWindows++;

		if (!m_isRunning)
		{
			auto lock = std::unique_lock{m_isRunningMutex};
			m_isRunningConditionVariable.wait(lock, [=]() -> bool { return m_isRunning; });
		}

		auto message = MSG{};
		while (GetMessageW(&message, 0, 0, 0))
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}

private:
	bool m_isOpen = false;
public:
	auto getIsOpen() const -> bool override
	{
		return m_isOpen;
	}

	auto create(
		std::string_view const p_title, 
		Avo::Point<> const p_position, 
		Avo::Size<> const p_size, 
		Avo::WindowStyleFlags const p_styleFlags, 
		Avo::Window* const p_parent
	) -> void override
	{
		m_title = p_title;
		m_crossPlatformStyles = p_styleFlags;
		m_styles = convertWindowStyleFlagsToWindowsWindowStyleFlags(p_styleFlags, p_parent);
		
		m_messageThread = std::thread{&WindowsWindow::thread_createAndRun, this, p_title, p_position, p_size, p_parent};
		if (!m_hasCreatedWindow)
		{
			auto lock = std::unique_lock{m_hasCreatedWindowMutex};
			m_hasCreatedWindowConditionVariable.wait(lock, [=]() -> bool { return m_hasCreatedWindow; });
		}
	}

	auto close() -> void override
	{
		if (m_windowHandle)
		{
			SendMessage(m_windowHandle, WM_CLOSE, 0, 0);
		}
	}

	//------------------------------

	auto enableUserInteraction() -> void override
	{
		PostMessage(m_windowHandle, WM_APP_SET_IS_ENABLED, 1, 0);
	}
	auto disableUserInteraction() -> void override
	{
		PostMessage(m_windowHandle, WM_APP_SET_IS_ENABLED, 0, 0);
	}
	auto getIsUserInteractionEnabled() -> bool override
	{
		return IsWindowEnabled(m_windowHandle);
	}

	//------------------------------

private:
	std::string m_title;
public:
	auto setTitle(std::string_view const p_title) -> void override
	{
		m_title = p_title;
		auto const wideTitle = Avo::convertUtf8ToUtf16(p_title);
		SetWindowTextW(m_windowHandle, reinterpret_cast<LPCWSTR>(wideTitle.c_str()));
	}
	auto getTitle() const -> std::string override
	{
		return m_title;
	}

	//------------------------------

	auto setStyles(Avo::WindowStyleFlags const p_styles) -> void override
	{
		SetWindowLongPtr(m_windowHandle, GWL_STYLE, convertWindowStyleFlagsToWindowsWindowStyleFlags(p_styles, GetParent(m_windowHandle)));
		SetWindowPos(m_windowHandle, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}
	auto getStyles() const -> Avo::WindowStyleFlags override
	{
		return m_crossPlatformStyles;
	}

	//------------------------------

	// Internal method used to initialize the window handle at the right moment.
	auto setNativeHandle(HWND const p_handle) -> void
	{
		m_windowHandle = p_handle;
	}
	auto getNativeHandle() const -> void* override
	{
		return m_windowHandle;
	}

	//------------------------------

private:
	bool m_isFullscreen = false;
	RECT m_windowRectBeforeFullscreen{};
	bool m_wasWindowMaximizedBeforeFullscreen = false;
public:
	auto setIsFullscreen(bool const p_isFullscreen) -> void override
	{
		if (m_isFullscreen == p_isFullscreen)
		{
			return;
		}
		if (p_isFullscreen)
		{
			m_wasWindowMaximizedBeforeFullscreen = false;
			if (m_state == Avo::WindowState::Restored)
			{
				GetWindowRect(m_windowHandle, &m_windowRectBeforeFullscreen);
			}
			else if (m_state == Avo::WindowState::Maximized)
			{
				m_wasWindowMaximizedBeforeFullscreen = true;
			}

			auto info = MONITORINFO{};
			info.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(MonitorFromWindow(m_windowHandle, MONITOR_DEFAULTTONEAREST), &info);
			SetWindowLongPtr(m_windowHandle, GWL_STYLE, WS_VISIBLE | WS_MAXIMIZE);
			SetWindowPos(
				m_windowHandle, 0, 
				info.rcMonitor.left, info.rcMonitor.top, 
				info.rcMonitor.right - info.rcMonitor.left, 
				info.rcMonitor.bottom - info.rcMonitor.top, 
				SWP_NOZORDER | SWP_NOOWNERZORDER
			);
		}
		else
		{
			SetWindowLongPtr(
				m_windowHandle, GWL_STYLE, 
				m_wasWindowMaximizedBeforeFullscreen*WS_MAXIMIZE | m_styles & ~(WS_MAXIMIZE | WS_MINIMIZE)
			);
			if (m_wasWindowMaximizedBeforeFullscreen)
			{
				SetWindowPos(
					m_windowHandle, 0, 0, 0, 0, 0, 
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
				);
			}
			else
			{
				SetWindowPos(
					m_windowHandle, 0, 
					m_windowRectBeforeFullscreen.left, m_windowRectBeforeFullscreen.top, 
					m_windowRectBeforeFullscreen.right - m_windowRectBeforeFullscreen.left, 
					m_windowRectBeforeFullscreen.bottom - m_windowRectBeforeFullscreen.top, 
					SWP_NOZORDER | SWP_NOOWNERZORDER
				);
			}
		}
		m_isFullscreen = p_isFullscreen;
	}
	auto switchFullscreen() -> void override
	{
		setIsFullscreen(!m_isFullscreen);
	}
	auto getIsFullscreen() const -> bool override
	{
		return m_isFullscreen;
	}

	//------------------------------

private:
	Avo::WindowState m_state = Avo::WindowState::Restored;
public:
	auto hide() -> void override
	{
		ShowWindow(m_windowHandle, SW_HIDE);
	}
	auto show() -> void override
	{
		ShowWindow(m_windowHandle, SW_SHOW);
	}

	auto maximize() -> void override
	{
		ShowWindow(m_windowHandle, SW_MAXIMIZE);
	}
	auto minimize() -> void override
	{
		ShowWindow(m_windowHandle, SW_MINIMIZE);
	}
	auto restore() -> void override
	{
		ShowWindow(m_windowHandle, SW_RESTORE);
	}

	auto setState(Avo::WindowState const p_state) -> void override
	{
		if (p_state == Avo::WindowState::Maximized)
			ShowWindow(m_windowHandle, SW_MAXIMIZE);
		else if (p_state == Avo::WindowState::Minimized)
			ShowWindow(m_windowHandle, SW_MINIMIZE);
		else if (p_state == Avo::WindowState::Restored)
			ShowWindow(m_windowHandle, SW_RESTORE);
	}
	auto getState() const -> Avo::WindowState override
	{
		return m_state;
	}

	//------------------------------

private:
	Avo::Point<Avo::Pixels> m_position;
public:
	auto setPosition(Avo::Point<Avo::Pixels> const p_position) -> void override
	{
		SetWindowPos(m_windowHandle, 0, p_position.x, p_position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		m_position = p_position;
	}
	auto getPosition() const -> Avo::Point<Avo::Pixels> override
	{
		return m_position;
	}
	auto getPositionX() const -> Avo::Pixels override
	{
		return m_position.x;
	}
	auto getPositionY() const -> Avo::Pixels override
	{
		return m_position.y;
	}

private:
	Avo::Size<Avo::Pixels> m_size;
public:
	auto setPixelSize(Avo::Size<Avo::Pixels> const p_size) -> void override
	{
		if (m_windowHandle)
		{
			if (getHasCustomBorder())
			{
				PostMessage(m_windowHandle, WM_APP_CHANGE_SIZE, static_cast<WPARAM>(p_size.width), static_cast<LPARAM>(p_size.height));
			}
			else
			{
				auto windowRect = RECT{0, 0, static_cast<int>(p_size.width), static_cast<int>(p_size.height)};
				AdjustWindowRect(&windowRect, m_styles, 0);
			
				PostMessage(
					m_windowHandle, 
					WM_APP_CHANGE_SIZE, 
					static_cast<WPARAM>(windowRect.right - windowRect.left), 
					static_cast<LPARAM>(windowRect.bottom - windowRect.top)
				);
			}
		}
	}
	auto getPixelSize() const -> Avo::Size<Avo::Pixels> override
	{
		return m_size;
	}
	auto getPixelWidth() const -> Avo::Pixels override
	{
		return m_size.x;
	}
	auto getPixelHeight() const -> Avo::Pixels override
	{
		return m_size.y;
	}

	//------------------------------

private:
	Avo::Size<Avo::Pixels> m_minSize;
public:
	auto setMinPixelSize(Avo::Size<Avo::Pixels> const p_size) -> void override
	{
		m_minSize = p_size;
	}
	auto getMinPixelSize() const -> Avo::Size<Avo::Pixels> override
	{
		return m_minSize;
	}
	auto getMinPixelWidth() const -> Avo::Pixels override
	{
		return m_minSize.x;
	}
	auto getMinPixelHeight() const -> Avo::Pixels override
	{
		return m_minSize.y;
	}

private:
	Avo::Size<Avo::Pixels> m_maxSize;
public:
	auto setMaxPixelSize(Avo::Size<Avo::Pixels> p_size) -> void override
	{
		m_maxSize = p_size;
	}
	auto getMaxPixelSize() const -> Avo::Size<Avo::Pixels> override
	{
		return m_maxSize;
	}
	auto getMaxPixelWidth() const -> Avo::Pixels override
	{
		return m_maxSize.x;
	}
	auto getMaxPixelHeight() const -> Avo::Pixels override
	{
		return m_maxSize.y;
	}

	//------------------------------

private:
	auto getMonitorInfo() const -> MONITORINFO
	{
		auto info = MONITORINFO{};
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(MonitorFromWindow(m_windowHandle, MONITOR_DEFAULTTONEAREST), &info);
		return info;
	}

public:
	auto getMonitorBounds() const -> Avo::Rectangle<Avo::Pixels> override
	{
		auto const rect = getMonitorInfo().rcMonitor;
		return {
			rect.left, rect.top, 
			rect.right, rect.bottom
		};
	}
	auto getMonitorPosition() const -> Avo::Point<Avo::Pixels> override
	{
		auto const rect = getMonitorInfo().rcMonitor;
		return {rect.left, rect.top};
	}
	auto getMonitorSize() const -> Avo::Size<Avo::Pixels> override
	{
		auto const rect = getMonitorInfo().rcMonitor;
		return {
			rect.right - rect.left, 
			rect.bottom - rect.top
		};
	}
	auto getMonitorWidth() const -> Avo::Pixels override
	{
		auto const rect = getMonitorInfo().rcMonitor;
		return rect.right - rect.left;
	}
	auto getMonitorHeight() const -> Avo::Pixels override
	{
		auto const rect = getMonitorInfo().rcMonitor;
		return rect.bottom - rect.top;
	}

	//------------------------------

	auto getWorkAreaBounds() const -> Avo::Rectangle<Avo::Pixels> override
	{
		auto const rect = getMonitorInfo().rcWork;
		return {
			rect.left, rect.top, 
			rect.right, rect.bottom 
		};
	}
	auto getWorkAreaPosition() const -> Avo::Point<Avo::Pixels> override
	{
		auto const rect = getMonitorInfo().rcWork;
		return {rect.left, rect.top};
	}
	auto getWorkAreaSize() const -> Avo::Size<Avo::Pixels> override
	{
		auto const rect = getMonitorInfo().rcWork;
		return {
			rect.right - rect.left, 
			rect.bottom - rect.top
		};
	}
	auto getWorkAreaWidth() const -> Avo::Pixels override
	{
		auto const rect = getMonitorInfo().rcWork;
		return rect.right - rect.left;
	}
	auto getWorkAreaHeight() const -> Avo::Pixels override
	{
		auto const rect = getMonitorInfo().rcWork;
		return rect.bottom - rect.top;
	}

	//------------------------------

private:
	static auto getIsVirtualKeyDown(int const p_key) -> bool
	{
		return GetAsyncKeyState(p_key) & (1 << 16);
	}

public:
	auto getIsKeyDown(Avo::KeyboardKey const p_key) const -> bool override
	{
		switch (p_key)
		{
			case Avo::KeyboardKey::A:
				return getIsVirtualKeyDown(0x41);
			case Avo::KeyboardKey::B:
				return getIsVirtualKeyDown(0x42);
			case Avo::KeyboardKey::C:
				return getIsVirtualKeyDown(0x43);
			case Avo::KeyboardKey::D:
				return getIsVirtualKeyDown(0x44);
			case Avo::KeyboardKey::E:
				return getIsVirtualKeyDown(0x45);
			case Avo::KeyboardKey::F:
				return getIsVirtualKeyDown(0x46);
			case Avo::KeyboardKey::G:
				return getIsVirtualKeyDown(0x47);
			case Avo::KeyboardKey::H:
				return getIsVirtualKeyDown(0x48);
			case Avo::KeyboardKey::I:
				return getIsVirtualKeyDown(0x49);
			case Avo::KeyboardKey::J:
				return getIsVirtualKeyDown(0x4A);
			case Avo::KeyboardKey::K:
				return getIsVirtualKeyDown(0x4B);
			case Avo::KeyboardKey::L:
				return getIsVirtualKeyDown(0x4C);
			case Avo::KeyboardKey::M:
				return getIsVirtualKeyDown(0x4D);
			case Avo::KeyboardKey::N:
				return getIsVirtualKeyDown(0x4E);
			case Avo::KeyboardKey::O:
				return getIsVirtualKeyDown(0x4F);
			case Avo::KeyboardKey::P:
				return getIsVirtualKeyDown(0x50);
			case Avo::KeyboardKey::Q:
				return getIsVirtualKeyDown(0x51);
			case Avo::KeyboardKey::R:
				return getIsVirtualKeyDown(0x52);
			case Avo::KeyboardKey::S:
				return getIsVirtualKeyDown(0x53);
			case Avo::KeyboardKey::T:
				return getIsVirtualKeyDown(0x54);
			case Avo::KeyboardKey::U:
				return getIsVirtualKeyDown(0x55);
			case Avo::KeyboardKey::V:
				return getIsVirtualKeyDown(0x56);
			case Avo::KeyboardKey::W:
				return getIsVirtualKeyDown(0x57);
			case Avo::KeyboardKey::X:
				return getIsVirtualKeyDown(0x58);
			case Avo::KeyboardKey::Y:
				return getIsVirtualKeyDown(0x59);
			case Avo::KeyboardKey::Z:
				return getIsVirtualKeyDown(0x5A);
			case Avo::KeyboardKey::Alt:
				return getIsVirtualKeyDown(VK_MENU);
			case Avo::KeyboardKey::Backspace:
				return getIsVirtualKeyDown(VK_BACK);
			case Avo::KeyboardKey::CapsLock:
				return getIsVirtualKeyDown(VK_CAPITAL);
			case Avo::KeyboardKey::Clear:
				return getIsVirtualKeyDown(VK_CLEAR);
			case Avo::KeyboardKey::Control:
				return getIsVirtualKeyDown(VK_CONTROL);
			case Avo::KeyboardKey::Decimal:
				return getIsVirtualKeyDown(VK_DECIMAL);
			case Avo::KeyboardKey::Delete:
				return getIsVirtualKeyDown(VK_DELETE);
			case Avo::KeyboardKey::Down:
				return getIsVirtualKeyDown(VK_DOWN);
			case Avo::KeyboardKey::End:
				return getIsVirtualKeyDown(VK_END);
			case Avo::KeyboardKey::Enter:
				return getIsVirtualKeyDown(VK_RETURN);
			case Avo::KeyboardKey::Escape:
				return getIsVirtualKeyDown(VK_ESCAPE);
			case Avo::KeyboardKey::F1:
				return getIsVirtualKeyDown(VK_F1);
			case Avo::KeyboardKey::F2:
				return getIsVirtualKeyDown(VK_F2);
			case Avo::KeyboardKey::F3:
				return getIsVirtualKeyDown(VK_F3);
			case Avo::KeyboardKey::F4:
				return getIsVirtualKeyDown(VK_F4);
			case Avo::KeyboardKey::F5:
				return getIsVirtualKeyDown(VK_F5);
			case Avo::KeyboardKey::F6:
				return getIsVirtualKeyDown(VK_F6);
			case Avo::KeyboardKey::F7:
				return getIsVirtualKeyDown(VK_F7);
			case Avo::KeyboardKey::F8:
				return getIsVirtualKeyDown(VK_F8);
			case Avo::KeyboardKey::F9:
				return getIsVirtualKeyDown(VK_F9);
			case Avo::KeyboardKey::F10:
				return getIsVirtualKeyDown(VK_F10);
			case Avo::KeyboardKey::F11:
				return getIsVirtualKeyDown(VK_F11);
			case Avo::KeyboardKey::F12:
				return getIsVirtualKeyDown(VK_F12);
			case Avo::KeyboardKey::F13:
				return getIsVirtualKeyDown(VK_F13);
			case Avo::KeyboardKey::F14:
				return getIsVirtualKeyDown(VK_F14);
			case Avo::KeyboardKey::F15:
				return getIsVirtualKeyDown(VK_F15);
			case Avo::KeyboardKey::F16:
				return getIsVirtualKeyDown(VK_F16);
			case Avo::KeyboardKey::F17:
				return getIsVirtualKeyDown(VK_F17);
			case Avo::KeyboardKey::F18:
				return getIsVirtualKeyDown(VK_F18);
			case Avo::KeyboardKey::F19:
				return getIsVirtualKeyDown(VK_F19);
			case Avo::KeyboardKey::F20:
				return getIsVirtualKeyDown(VK_F20);
			case Avo::KeyboardKey::F21:
				return getIsVirtualKeyDown(VK_F21);
			case Avo::KeyboardKey::F22:
				return getIsVirtualKeyDown(VK_F22);
			case Avo::KeyboardKey::F23:
				return getIsVirtualKeyDown(VK_F23);
			case Avo::KeyboardKey::F24:
				return getIsVirtualKeyDown(VK_F24);
			case Avo::KeyboardKey::Help:
				return getIsVirtualKeyDown(VK_HELP);
			case Avo::KeyboardKey::Home:
				return getIsVirtualKeyDown(VK_HOME);
			case Avo::KeyboardKey::Insert:
				return getIsVirtualKeyDown(VK_INSERT);
			case Avo::KeyboardKey::Left:
				return getIsVirtualKeyDown(VK_LEFT);
			case Avo::KeyboardKey::Menu:
				return getIsVirtualKeyDown(VK_APPS);
			case Avo::KeyboardKey::Number0:
				return getIsVirtualKeyDown(0x30);
			case Avo::KeyboardKey::Number1:
				return getIsVirtualKeyDown(0x31);
			case Avo::KeyboardKey::Number2:
				return getIsVirtualKeyDown(0x32);
			case Avo::KeyboardKey::Number3:
				return getIsVirtualKeyDown(0x33);
			case Avo::KeyboardKey::Number4:
				return getIsVirtualKeyDown(0x34);
			case Avo::KeyboardKey::Number5:
				return getIsVirtualKeyDown(0x35);
			case Avo::KeyboardKey::Number6:
				return getIsVirtualKeyDown(0x36);
			case Avo::KeyboardKey::Number7:
				return getIsVirtualKeyDown(0x37);
			case Avo::KeyboardKey::Number8:
				return getIsVirtualKeyDown(0x38);
			case Avo::KeyboardKey::Number9:
				return getIsVirtualKeyDown(0x39);
			case Avo::KeyboardKey::NumLock:
				return getIsVirtualKeyDown(VK_NUMLOCK);
			case Avo::KeyboardKey::Numpad0:
				return getIsVirtualKeyDown(VK_NUMPAD0);
			case Avo::KeyboardKey::Numpad1:
				return getIsVirtualKeyDown(VK_NUMPAD1);
			case Avo::KeyboardKey::Numpad2:
				return getIsVirtualKeyDown(VK_NUMPAD2);
			case Avo::KeyboardKey::Numpad3:
				return getIsVirtualKeyDown(VK_NUMPAD3);
			case Avo::KeyboardKey::Numpad4:
				return getIsVirtualKeyDown(VK_NUMPAD4);
			case Avo::KeyboardKey::Numpad5:
				return getIsVirtualKeyDown(VK_NUMPAD5);
			case Avo::KeyboardKey::Numpad6:
				return getIsVirtualKeyDown(VK_NUMPAD6);
			case Avo::KeyboardKey::Numpad7:
				return getIsVirtualKeyDown(VK_NUMPAD7);
			case Avo::KeyboardKey::Numpad8:
				return getIsVirtualKeyDown(VK_NUMPAD8);
			case Avo::KeyboardKey::Numpad9:
				return getIsVirtualKeyDown(VK_NUMPAD9);
			case Avo::KeyboardKey::Add:
				return getIsVirtualKeyDown(VK_ADD);
			case Avo::KeyboardKey::Subtract:
				return getIsVirtualKeyDown(VK_SUBTRACT);
			case Avo::KeyboardKey::Multiply:
				return getIsVirtualKeyDown(VK_MULTIPLY);
			case Avo::KeyboardKey::Divide:
				return getIsVirtualKeyDown(VK_DIVIDE);
			case Avo::KeyboardKey::PageDown:
				return getIsVirtualKeyDown(VK_NEXT);
			case Avo::KeyboardKey::PageUp:
				return getIsVirtualKeyDown(VK_PRIOR);
			case Avo::KeyboardKey::Pause:
				return getIsVirtualKeyDown(VK_PAUSE);
			case Avo::KeyboardKey::PrintScreen:
				return getIsVirtualKeyDown(VK_SNAPSHOT);
			case Avo::KeyboardKey::Comma:
				return getIsVirtualKeyDown(VK_OEM_COMMA);
			case Avo::KeyboardKey::Period:
				return getIsVirtualKeyDown(VK_OEM_PERIOD);
			case Avo::KeyboardKey::Plus:
				return getIsVirtualKeyDown(VK_OEM_PLUS);
			case Avo::KeyboardKey::Minus:
				return getIsVirtualKeyDown(VK_OEM_MINUS);
			case Avo::KeyboardKey::Regional1:
				return getIsVirtualKeyDown(VK_OEM_1);
			case Avo::KeyboardKey::Regional2:
				return getIsVirtualKeyDown(VK_OEM_2);
			case Avo::KeyboardKey::Regional3:
				return getIsVirtualKeyDown(VK_OEM_3);
			case Avo::KeyboardKey::Regional4:
				return getIsVirtualKeyDown(VK_OEM_4);
			case Avo::KeyboardKey::Regional5:
				return getIsVirtualKeyDown(VK_OEM_5);
			case Avo::KeyboardKey::Regional6:
				return getIsVirtualKeyDown(VK_OEM_6);
			case Avo::KeyboardKey::Regional7:
				return getIsVirtualKeyDown(VK_OEM_7);
			case Avo::KeyboardKey::Right:
				return getIsVirtualKeyDown(VK_RIGHT);
			case Avo::KeyboardKey::Separator:
				return getIsVirtualKeyDown(VK_SEPARATOR);
			case Avo::KeyboardKey::Shift:
				return getIsVirtualKeyDown(VK_SHIFT);
			case Avo::KeyboardKey::Spacebar:
				return getIsVirtualKeyDown(VK_SPACE);
			case Avo::KeyboardKey::Tab:
				return getIsVirtualKeyDown(VK_TAB);
			case Avo::KeyboardKey::Up:
				return getIsVirtualKeyDown(VK_UP);
		}
		return false;
	}
	auto getIsMouseButtonDown(Avo::MouseButton const p_button) const -> bool override
	{
		switch (p_button)
		{
			case Avo::MouseButton::Left:
				return getIsVirtualKeyDown(VK_LBUTTON);
			case Avo::MouseButton::Middle:
				return getIsVirtualKeyDown(VK_MBUTTON);
			case Avo::MouseButton::Right:
				return getIsVirtualKeyDown(VK_RBUTTON);
			case Avo::MouseButton::X0:
				return getIsVirtualKeyDown(VK_XBUTTON1);
			case Avo::MouseButton::X1:
				return getIsVirtualKeyDown(VK_XBUTTON2);
		}
		return false;
	}

private:
	Avo::Point<int32> m_mousePosition{-1, -1};
public:
	auto getMousePosition() const -> Avo::Point<> override
	{
		return m_mousePosition/m_dipToPixelFactor;
	}

	//------------------------------

private:
	std::unordered_map<wchar_t const*, HCURSOR> m_cursors;
	Avo::Cursor m_cursorType = Avo::Cursor::Unknown;
	HCURSOR m_cursorHandle;

public:
	auto setCursor(Avo::Cursor const p_cursor) -> void override
	{
		if (p_cursor == m_cursorType) return;

		wchar_t const* name = nullptr;
		switch (p_cursor)
		{
			case Avo::Cursor::Arrow:
				name = IDC_ARROW;
				break;
			case Avo::Cursor::Blocked:
				name = IDC_NO;
				break;
			case Avo::Cursor::Hand:
				name = IDC_HAND;
				break;
			case Avo::Cursor::Ibeam:
				name = IDC_IBEAM;
				break;
			case Avo::Cursor::ResizeAll:
				name = IDC_SIZEALL;
				break;
			case Avo::Cursor::ResizeNESW:
				name = IDC_SIZENESW;
				break;
			case Avo::Cursor::ResizeNS:
				name = IDC_SIZENS;
				break;
			case Avo::Cursor::ResizeNWSE:
				name = IDC_SIZENWSE;
				break;
			case Avo::Cursor::ResizeWE:
				name = IDC_SIZEWE;
				break;
			case Avo::Cursor::Wait:
				name = IDC_WAIT;
				break;
		}
		
		if (auto const position = m_cursors.find(name);
		    position == m_cursors.end())
		{
			auto const newCursor = LoadCursorW(nullptr, name);
			m_cursors.insert({name, newCursor});
			m_cursorHandle = newCursor;
			if (!m_isMouseOutsideClientArea)
			{
				SetCursor(m_cursorHandle);
			}
		}
		else
		{
			m_cursorHandle = position->second;
			if (!m_isMouseOutsideClientArea)
			{
				SetCursor(m_cursorHandle);
			}
		}
		m_cursorType = p_cursor;
	}
	auto getCursor() const -> Avo::Cursor override
	{
		return m_cursorType;
	}

	//------------------------------

private:
	ComReference<OleDropSource> m_oleDropSource;
	ComReference<OleDropTarget> m_oleDropTarget;

	CLIPFORMAT m_clipboardFormat_fileContents{};
	CLIPFORMAT m_clipboardFormat_fileGroupDescriptor{};
	CLIPFORMAT m_clipboardFormat_additionalData{};

	/*
		Assumes that p_medium is zero-initialized.
	*/
	auto createAdditionalData(FORMATETC& p_format, STGMEDIUM& p_medium, uint64 const p_data) const -> void
	{
		p_format = {
			/*.cfFormat = */m_clipboardFormat_additionalData,
			/*.ptd = */nullptr,
			/*.dwAspect = */DVASPECT_CONTENT,
			/*.lindex = */-1,
			/*.tymed = */TYMED_HGLOBAL
		};
		p_medium.tymed = TYMED_HGLOBAL;
		p_medium.hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(p_data));
		*static_cast<uint64*>(p_medium.hGlobal) = p_data;
	}
	auto createStringOleDataObject(std::u16string_view const p_string, uint64 const p_additionalData) const -> ComReference<OleDataObject>
	{
		auto formats = std::array<FORMATETC, 2>{};
		formats[0].cfFormat = CF_UNICODETEXT;
		formats[0].tymed = TYMED_HGLOBAL;
		formats[0].dwAspect = DVASPECT_CONTENT;
		formats[0].lindex = -1;

		auto mediums = std::array<STGMEDIUM, 2>{};
		mediums[0].tymed = TYMED_HGLOBAL;

		//------------------------------

		auto const stringSize = (p_string.size() + 1)*sizeof(wchar_t);
		mediums[0].hGlobal = GlobalAlloc(GMEM_FIXED, stringSize);
		memcpy(mediums[0].hGlobal, p_string.data(), stringSize);

		//------------------------------

		createAdditionalData(formats[1], mediums[1], p_additionalData);

		return makeComReference<OleDataObject>(formats, mediums);
	}
	auto createImageOleDataObject(Avo::Image const& p_image, uint64 const p_additionalData) const -> ComReference<OleDataObject>
	{
		auto formats = std::array<FORMATETC, 3>{};
		auto mediums = std::array<STGMEDIUM, 3>{};

		formats[0].cfFormat = m_clipboardFormat_fileContents;
		formats[0].tymed = TYMED_ISTREAM;
		formats[0].dwAspect = DVASPECT_CONTENT;
		formats[0].lindex = -1;

		mediums[0].tymed = TYMED_ISTREAM;
		mediums[0].pstm = static_cast<IStream*>(m_gui->getDrawingContext()->createImageFileDataNativeStream(p_image));

		//------------------------------

		formats[1].cfFormat = m_clipboardFormat_fileGroupDescriptor;
		formats[1].tymed = TYMED_HGLOBAL;
		formats[1].dwAspect = DVASPECT_CONTENT;
		formats[1].lindex = -1;

		mediums[1].tymed = TYMED_HGLOBAL;
		mediums[1].hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(FILEGROUPDESCRIPTORW));

		auto groupDescriptor = static_cast<FILEGROUPDESCRIPTORW*>(mediums[1].hGlobal);
		groupDescriptor->cItems = 1;
		groupDescriptor->fgd[0].dwFlags = FD_UNICODE;
		wcscpy_s(groupDescriptor->fgd[0].cFileName, L"image.png");

		//------------------------------

		createAdditionalData(formats[2], mediums[2], p_additionalData);

		return makeComReference<OleDataObject>(formats, mediums);
	}
	auto createFileOleDataObject(
		Avo::DataView const p_data, 
		std::u16string_view const p_name, 
		uint64 const p_additionalData
	) const -> ComReference<OleDataObject>
	{
		auto formats = std::array<FORMATETC, 4>{};
		auto mediums = std::array<STGMEDIUM, 4>{};

		formats[0].cfFormat = CF_UNICODETEXT;
		formats[0].tymed = TYMED_HGLOBAL;
		formats[0].dwAspect = DVASPECT_CONTENT;
		formats[0].lindex = -1;

		mediums[0].tymed = TYMED_HGLOBAL;

		auto const stringSize = (p_name.size() + 1)*sizeof(wchar_t);
		mediums[0].hGlobal = GlobalAlloc(GMEM_FIXED, stringSize);
		memcpy(mediums[0].hGlobal, p_name.data(), stringSize);

		//------------------------------

		formats[1].cfFormat = m_clipboardFormat_fileContents;
		formats[1].tymed = TYMED_ISTREAM;
		formats[1].dwAspect = DVASPECT_CONTENT;
		formats[1].lindex = -1;

		mediums[1].tymed = TYMED_ISTREAM;
		mediums[1].pstm = SHCreateMemStream(reinterpret_cast<BYTE const*>(p_data.data()), p_data.size());

		//------------------------------

		formats[2].cfFormat = m_clipboardFormat_fileGroupDescriptor;
		formats[2].tymed = TYMED_HGLOBAL;
		formats[2].dwAspect = DVASPECT_CONTENT;
		formats[2].lindex = -1;

		mediums[2].tymed = TYMED_HGLOBAL;
		mediums[2].hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(FILEGROUPDESCRIPTORW));

		auto groupDescriptor = static_cast<FILEGROUPDESCRIPTORW*>(mediums[2].hGlobal);
		groupDescriptor->cItems = 1;
		groupDescriptor->fgd[0].dwFlags = FD_UNICODE;
		memcpy(groupDescriptor->fgd[0].cFileName, p_name.data(), stringSize);

		//------------------------------

		createAdditionalData(formats[3], mediums[3], p_additionalData);

		return makeComReference<OleDataObject>(formats, mediums);
	}
	auto createFileOleDataObject(std::u16string_view const p_path, uint64 const p_additionalData) const -> ComReference<OleDataObject>
	{
		auto const path = filesystem::path{p_path};
		auto const widePathStringSize = (p_path.size() + 1)*sizeof(wchar_t);

		auto formats = std::array<FORMATETC, 6>{};
		auto mediums = std::array<STGMEDIUM, 6>{};

		formats[0].cfFormat = CF_UNICODETEXT;
		formats[0].tymed = TYMED_HGLOBAL;
		formats[0].dwAspect = DVASPECT_CONTENT;
		formats[0].lindex = -1;

		mediums[0].tymed = TYMED_HGLOBAL;
		mediums[0].hGlobal = GlobalAlloc(GMEM_FIXED, widePathStringSize);
		memcpy(mediums[0].hGlobal, p_path.data(), widePathStringSize);

		//------------------------------

		formats[1].cfFormat = CF_HDROP;
		formats[1].tymed = TYMED_HGLOBAL;
		formats[1].dwAspect = DVASPECT_CONTENT;
		formats[1].lindex = -1;

		mediums[1].tymed = TYMED_HGLOBAL;
		mediums[1].hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(DROPFILES) + widePathStringSize + sizeof(wchar_t));

		auto filenameStructure = static_cast<DROPFILES*>(mediums[1].hGlobal);
		filenameStructure->fNC = true;
		filenameStructure->fWide = true;
		filenameStructure->pt.x = 0;
		filenameStructure->pt.y = 0;
		filenameStructure->pFiles = sizeof(DROPFILES);

		memcpy(static_cast<char*>(mediums[1].hGlobal) + sizeof(DROPFILES), p_path.data(), widePathStringSize);
		*reinterpret_cast<wchar_t*>(static_cast<char*>(mediums[1].hGlobal) + sizeof(DROPFILES) + widePathStringSize) = 0;

		//------------------------------

		createAdditionalData(formats[2], mediums[2], p_additionalData);

		//------------------------------

		if (filesystem::is_regular_file(path))
		{
			formats[3].cfFormat = m_clipboardFormat_fileContents;
			formats[3].tymed = TYMED_ISTREAM;
			formats[3].dwAspect = DVASPECT_CONTENT;
			formats[3].lindex = -1;

			mediums[3].tymed = TYMED_ISTREAM;
			SHCreateStreamOnFileEx(reinterpret_cast<LPCWSTR>(p_path.data()), STGM_READ | STGM_SHARE_DENY_WRITE, 0, false, 0, &mediums[3].pstm);

			//------------------------------

			formats[4].cfFormat = m_clipboardFormat_fileGroupDescriptor;
			formats[4].tymed = TYMED_HGLOBAL;
			formats[4].dwAspect = DVASPECT_CONTENT;
			formats[4].lindex = -1;

			mediums[4].tymed = TYMED_HGLOBAL;
			mediums[4].hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(FILEGROUPDESCRIPTORW));

			auto groupDescriptor = static_cast<FILEGROUPDESCRIPTORW*>(mediums[4].hGlobal);
			groupDescriptor->cItems = 1;
			groupDescriptor->fgd[0].dwFlags = FD_UNICODE;

			memcpy(groupDescriptor->fgd[0].cFileName, p_path.data(), widePathStringSize);

			return makeComReference<OleDataObject>(formats, mediums);
		}
		return makeComReference<OleDataObject>(formats, mediums);
	}
	auto createFilesOleDataObject(
		Range<std::u16string*> const p_pathStrings, 
		uint64 const p_additionalData
	) const -> ComReference<OleDataObject>
	{
		auto formats = std::array<FORMATETC, 2>{};
		auto mediums = std::array<STGMEDIUM, 2>{};

		//------------------------------
		// Create an HDROP format, which is just the paths of all items.

		formats[0].cfFormat = CF_HDROP;
		formats[0].tymed = TYMED_HGLOBAL;
		formats[0].dwAspect = DVASPECT_CONTENT;
		formats[0].lindex = -1;

		auto pathsStringSize = Count{};
		for (auto const& pathString : p_pathStrings)
		{
			pathsStringSize += pathString.size() + 1;
		}
		pathsStringSize++;

		mediums[0].tymed = TYMED_HGLOBAL;
		mediums[0].hGlobal = GlobalAlloc(GMEM_FIXED, sizeof(DROPFILES) + pathsStringSize*sizeof(wchar_t));

		auto filenameStructure = static_cast<DROPFILES*>(mediums[0].hGlobal);
		filenameStructure->fNC = true;
		filenameStructure->fWide = true;
		filenameStructure->pt.x = 0;
		filenameStructure->pt.y = 0;
		filenameStructure->pFiles = sizeof(DROPFILES);

		auto pathsString = reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(mediums[0].hGlobal) + sizeof(DROPFILES));
		auto pathsStringPosition = pathsString;
		for (auto const& pathString : p_pathStrings)
		{
			std::copy(pathString.cbegin(), pathString.cend(), pathsStringPosition);
			pathsStringPosition[pathString.size()] = 0;
			//memcpy(pathsStringPosition, pathString.data(), (pathString.size() + 1)*sizeof(wchar_t));
			pathsStringPosition += pathString.size() + 1;
		}
		pathsString[pathsStringSize - 1] = 0;

		//------------------------------

		createAdditionalData(formats[1], mediums[1], p_additionalData);

		//------------------------------

		return makeComReference<OleDataObject>(formats, mediums);
	}

	auto doDragDrop(
		OleDataObject* const p_dataObject, 
		Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition
	) -> DWORD
	{
		auto const mousePositionBefore = m_mousePosition;

		auto event = Avo::MouseEvent{};
		event.xy = m_mousePosition/m_dipToPixelFactor;
		if (getIsMouseButtonDown(Avo::MouseButton::Left))
		{
			event.mouseButton = Avo::MouseButton::Left;
		}
		else if (getIsMouseButtonDown(Avo::MouseButton::Middle))
		{
			event.mouseButton = Avo::MouseButton::Middle;
		}
		else if (getIsMouseButtonDown(Avo::MouseButton::Right))
		{
			event.mouseButton = Avo::MouseButton::Right;
		}
		else if (getIsMouseButtonDown(Avo::MouseButton::X0))
		{
			event.mouseButton = Avo::MouseButton::X0;
		}
		else if (getIsMouseButtonDown(Avo::MouseButton::X1))
		{
			event.mouseButton = Avo::MouseButton::X1;
		}
		if (event.mouseButton != Avo::MouseButton::None)
		{
			m_gui->handleGlobalMouseUp(event);
		}

		if (p_dragImage)
		{
			m_oleDropSource->setDragImage(p_dragImage, p_dragImageCursorPosition, p_dataObject);
		}

		m_gui->unlockThreads();
		auto dropOperation = DWORD{DROPEFFECT_NONE};
		DoDragDrop(p_dataObject, m_oleDropSource, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK, &dropOperation);
		m_gui->lockThreads();

		event.xy = pixelsToDips(m_mousePosition);
		event.movement = Avo::Vector2d{(m_mousePosition - mousePositionBefore)/m_dipToPixelFactor};
		event.mouseButton = Avo::MouseButton::None;
		m_gui->handleGlobalMouseMove(event);

		return dropOperation;
	}

	auto convertNativeDropEffectToDragDropOperation(uint32 const p_dropEffect) 
		-> Avo::DragDropOperation
	{
		switch (p_dropEffect)
		{
		case DROPEFFECT_COPY:
			return Avo::DragDropOperation::Copy;
		case DROPEFFECT_MOVE:
			return Avo::DragDropOperation::Move;
		case DROPEFFECT_LINK:
			return Avo::DragDropOperation::Link;
		}
		return Avo::DragDropOperation::None;
	}

public:
	auto dragAndDropString(
		std::string_view const p_string, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		return dragAndDropString(Avo::convertUtf8ToUtf16(p_string), p_dragImage, p_dragImageCursorPosition, p_additionalData);
	}
	auto dragAndDropString(
		std::u16string_view const p_string, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		auto const dataObject = createStringOleDataObject(p_string, p_additionalData);
		auto const dropOperation = doDragDrop(dataObject, p_dragImage, p_dragImageCursorPosition);

		return convertNativeDropEffectToDragDropOperation(dropOperation);
	}

	auto dragAndDropImage(
		Avo::Image const& p_image, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		auto const dataObject = createImageOleDataObject(p_image, p_additionalData);
		auto const dropOperation = doDragDrop(dataObject, p_dragImage, p_dragImageCursorPosition);

		return convertNativeDropEffectToDragDropOperation(dropOperation);
	}

	auto dragAndDropFile(
		Avo::DataView const p_data, std::string_view const p_name, 
		Avo::Image const& p_dragImage, Avo::Point<> const p_dragImageCursorPosition, 
		uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		return dragAndDropFile(p_data, Avo::convertUtf8ToUtf16(p_name), p_dragImage, p_dragImageCursorPosition, p_additionalData);
	}
	auto dragAndDropFile(
		Avo::DataView const p_data, std::u16string_view const p_name, 
		Avo::Image const& p_dragImage, Avo::Point<> const p_dragImageCursorPosition, 
		uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		auto const dataObject = createFileOleDataObject(p_data, p_name, p_additionalData);
		auto const dropOperation = doDragDrop(dataObject, p_dragImage, p_dragImageCursorPosition);

		return convertNativeDropEffectToDragDropOperation(dropOperation);
	}

	auto dragAndDropFile(
		std::string_view const p_path, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		return dragAndDropFile(Avo::convertUtf8ToUtf16(p_path), p_dragImage, p_dragImageCursorPosition, p_additionalData);
	}
	auto dragAndDropFile(
		std::u16string_view const p_path, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		auto const dataObject = createFileOleDataObject(p_path, p_additionalData);
		auto const dropOperation = doDragDrop(dataObject, p_dragImage, p_dragImageCursorPosition);

		return convertNativeDropEffectToDragDropOperation(dropOperation);
	}

	auto dragAndDropFiles(
		Range<std::string*> const p_pathStrings, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		auto widePathStrings = std::vector<std::u16string>(p_pathStrings.size());
		for (auto const a : Indices{p_pathStrings})
		{
			widePathStrings[a] = Avo::convertUtf8ToUtf16(p_pathStrings[a]);
		}

		return dragAndDropFiles(Range{widePathStrings}, p_dragImage, p_dragImageCursorPosition, p_additionalData);
	}
	auto dragAndDropFiles(
		Range<std::u16string*> const p_pathStrings, Avo::Image const& p_dragImage, 
		Avo::Point<> const p_dragImageCursorPosition, uint64 const p_additionalData
	) -> Avo::DragDropOperation override
	{
		auto const dataObject = createFilesOleDataObject(p_pathStrings, p_additionalData);
		auto const dropOperation = doDragDrop(dataObject, p_dragImage, p_dragImageCursorPosition);

		return convertNativeDropEffectToDragDropOperation(dropOperation);
	}

	//------------------------------

	auto setClipboardString(std::string_view const p_string, uint64 const p_additionalData) const -> void override
	{
		setClipboardString(Avo::convertUtf8ToUtf16(p_string), p_additionalData);
	}
	auto setClipboardString(std::u16string_view const p_string, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createStringOleDataObject(p_string, p_additionalData);
		OleSetClipboard(dataObject);
	}

	auto setClipboardImage(Avo::Image const& p_image, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createImageOleDataObject(p_image, p_additionalData);
		OleSetClipboard(dataObject);
	}

	auto setClipboardFile(Avo::DataView const p_data, std::string_view const p_name, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createFileOleDataObject(p_data, Avo::convertUtf8ToUtf16(p_name), p_additionalData);
		OleSetClipboard(dataObject);
	}
	auto setClipboardFile(Avo::DataView const p_data, std::u16string_view const p_name, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createFileOleDataObject(p_data, p_name, p_additionalData);
		OleSetClipboard(dataObject);
	}

	auto setClipboardFile(std::string_view const p_path, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createFileOleDataObject(Avo::convertUtf8ToUtf16(p_path), p_additionalData);
		OleSetClipboard(dataObject);
	}
	auto setClipboardFile(std::u16string_view const p_path, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createFileOleDataObject(p_path, p_additionalData);
		OleSetClipboard(dataObject);
	}

	auto setClipboardFiles(Range<std::string*> const p_paths, uint64 const p_additionalData) const -> void override
	{
		auto widePathStrings = std::vector<std::u16string>(p_paths.size());
		for (auto const a : Indices{p_paths})
		{
			widePathStrings[a] = Avo::convertUtf8ToUtf16(p_paths[a]);
		}

		auto const dataObject = createFilesOleDataObject(widePathStrings, p_additionalData);
		OleSetClipboard(dataObject);
	}
	auto setClipboardFiles(Range<std::u16string*> const p_paths, uint64 const p_additionalData) const -> void override
	{
		auto const dataObject = createFilesOleDataObject(p_paths, p_additionalData);
		OleSetClipboard(dataObject);
	}

	auto getClipboardData() const -> std::unique_ptr<Avo::ClipboardData> override
	{
		auto dataObject = ComReference<IDataObject>{};
		OleGetClipboard(&dataObject);

		auto clipboardData = std::make_unique<OleClipboardData>(m_gui);
		clipboardData->setOleDataObject(dataObject);
		return clipboardData;
	}

	//------------------------------

private:
	bool m_hasGottenInitialSizeMessageForCustomBorderWindows{};
	bool m_isMouseOutsideClientArea = true;
	bool m_isDoubleClicking{};

	auto createMouseButtonEvent(
		WPARAM const p_wparam, LPARAM const p_lparam, 
		Avo::MouseButton const p_button,
		bool const p_isDoubleClick = false
	) -> Avo::MouseEvent
	{
		Avo::MouseEvent mouseEvent;
		mouseEvent.xy = pixelsToDips(Avo::Point{GET_X_LPARAM(p_lparam), GET_Y_LPARAM(p_lparam)});
		mouseEvent.mouseButton = p_button;
		mouseEvent.modifierKeys = convertWindowsKeyStateToModifierKeyFlags(p_wparam);
		return mouseEvent;
	}
	static auto getIsKeyRepeated(LPARAM const p_lparam) -> bool
	{
		return p_lparam & (1 << 30);
	}
	
public:
	// Returns true if the event was handled
	auto handleEvent(
		UINT const p_message, 
		WPARAM const p_data_a, LPARAM const p_data_b
	) -> long long
	{
		switch (p_message)
		{
			case WM_CREATE:
			{
				OleInitialize(0);

				m_oleDropSource = makeComReference<OleDropSource>(m_gui);
				m_oleDropTarget = makeComReference<OleDropTarget>(m_gui);
				RegisterDragDrop(m_windowHandle, *m_oleDropTarget);

				m_clipboardFormat_fileContents = RegisterClipboardFormatW(CFSTR_FILECONTENTS);
				m_clipboardFormat_fileGroupDescriptor = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
				m_clipboardFormat_additionalData = RegisterClipboardFormatW(CLIPBOARD_FORMAT_ADDITIONAL_DATA);

				//------------------------------

				EnableNonClientDpiScaling(m_windowHandle);

				/*
					LCS_WINDOWS_COLOR_SPACE is the default colorspace, but we want the background erase 
					color to be consistent with the colors of Direct2D and other potential graphics APIs 
					so it is changed to the sRGB color space.
				*/
				auto colorSpaceSettings = LOGCOLORSPACEW{
					/*.lcsSignature = */LCS_SIGNATURE,
					/*.lcsVersion = */0x400,
					/*.lcsSize = */sizeof(LOGCOLORSPACEW),
					/*.lcsCSType = */LCS_sRGB,
					/*.lcsIntent = */LCS_GM_ABS_COLORIMETRIC
				};

				auto colorSpace = CreateColorSpaceW(&colorSpaceSettings);
				SetColorSpace(GetDC(m_windowHandle), colorSpace);

				m_isOpen = true;

				windowCreateListeners(Avo::WindowEvent{this, m_size/m_dipToPixelFactor});

				return 0;
			}
			case WM_SIZE:
			{
				if (!m_hasCreatedWindow)
				{
					m_hasCreatedWindowMutex.lock();
					m_hasCreatedWindow = true;
					m_hasCreatedWindowMutex.unlock();
					m_hasCreatedWindowConditionVariable.notify_one();
					return 0;
				}

				Avo::WindowEvent windowEvent;
				windowEvent.window = this;
				if (p_data_a == SIZE_MINIMIZED)
				{
					windowMinimizeListeners(windowEvent);
					m_state = Avo::WindowState::Minimized;
				}
				else if (m_hasGottenInitialSizeMessageForCustomBorderWindows || !getHasCustomBorder())
				{
					m_size = {p_data_b & 0xffff, p_data_b >> 16 & 0xffff};
					windowEvent.size = pixelsToDips(m_size);

					if (p_data_a == SIZE_MAXIMIZED)
					{
						windowMaximizeListeners(windowEvent);
						m_state = Avo::WindowState::Maximized;
					}
					else if (p_data_a == SIZE_RESTORED && m_state != Avo::WindowState::Restored)
					{
						windowRestoreListeners(windowEvent);
						m_state = Avo::WindowState::Restored;
					}
					windowSizeChangeListeners(windowEvent);
				}
				m_hasGottenInitialSizeMessageForCustomBorderWindows = true;

				return 0;
			}
			case WM_APP_CHANGE_SIZE:
			{
				SetWindowPos(m_windowHandle, 0, 0, 0, (uint32)p_data_a, (uint32)p_data_b, SWP_NOMOVE | SWP_NOZORDER);

				return 0;
			}
			case WM_APP_SET_IS_ENABLED:
			{
				if (p_data_a)
				{
					EnableWindow(m_windowHandle, true);
					SetForegroundWindow(m_windowHandle);
				}
				else
				{
					auto child = GetWindow(m_windowHandle, GW_HWNDFIRST);
					if (child)
					{
						SetForegroundWindow(child);
						//SetActiveWindow(child);
						//SetFocus(child);
					}

					EnableWindow(m_windowHandle, false);
				}

				return 0;
			}
			case WM_ERASEBKGND:
			{
				auto deviceContext = reinterpret_cast<HDC>(p_data_a);

				auto rectangle = RECT{};
				GetUpdateRect(m_windowHandle, &rectangle, false);
				auto color = m_gui->getDrawingContext()->getBackgroundColor(); // Thread safe I think?
				FillRect(deviceContext, &rectangle, CreateSolidBrush(RGB(color.red*255, color.green*255, color.blue*255)));

				return 1; // We erased it.
			}
			case WM_NCCALCSIZE:
			{
				if (getHasCustomBorder() && p_data_a)
				{
					if (IsMaximized(m_windowHandle))
					{
						auto const parameters = reinterpret_cast<NCCALCSIZE_PARAMS*>(p_data_b);
						auto info = MONITORINFO{};
						info.cbSize = sizeof(MONITORINFO);
						GetMonitorInfo(MonitorFromRect(parameters->rgrc, MONITOR_DEFAULTTONEAREST), &info);

						parameters->rgrc[0] = info.rcWork;
					}
					return 0;
				}
				break;
			}
			case WM_NCMOUSEMOVE:
			{
				if (getHasCustomBorder() && GetCapture() != m_windowHandle)
				{
					auto mousePosition = POINT{GET_X_LPARAM(p_data_b), GET_Y_LPARAM(p_data_b)};
					ScreenToClient(m_windowHandle, &mousePosition);

					bool wasMousePositionInsideWindow = m_mousePosition.x >= 0 && m_mousePosition.y >= 0 
					                                 && m_mousePosition.x < m_size.x && m_mousePosition.y < m_size.y;
					if (!m_isMouseOutsideClientArea || m_isMouseOutsideClientArea && !wasMousePositionInsideWindow) // Is was outside of the nonclient area before this mousemove.
					{
						auto trackStructure = TRACKMOUSEEVENT{};
						trackStructure.dwFlags = TME_LEAVE | TME_NONCLIENT;
						trackStructure.cbSize = sizeof(TRACKMOUSEEVENT);
						trackStructure.hwndTrack = m_windowHandle;
						TrackMouseEvent(&trackStructure);
						if (m_isMouseOutsideClientArea)
						{
							// The window will recieve WM_MOUSELEAVE - no need for extra mouse events, so return.
							m_isMouseOutsideClientArea = true;
							return 0;
						}
					}

					m_isMouseOutsideClientArea = true;

					// We want the GUI to recieve mouse move events even when the mouse is inside the nonclient area of the window - 
					// because it is in this case part of the GUI (since the CustomBorder style flag is true).
					if (mousePosition.x >= 0 && mousePosition.y >= 0 && mousePosition.x < m_size.x && mousePosition.y < m_size.y)
					{
						auto mouseEvent = Avo::MouseEvent{};
						mouseEvent.xy = pixelsToDips(Avo::Point<Avo::Pixels>{mousePosition.x, mousePosition.y});
						mouseEvent.movement = Avo::Vector2d{mouseEvent.xy - pixelsToDips(m_mousePosition)};

						m_mousePosition = {mousePosition.x, mousePosition.y};

						auto lock = m_gui->createThreadLock();
						m_gui->handleGlobalMouseMove(mouseEvent);
					}
					return 0;
				}
				break;
			}
			case WM_MOUSEMOVE:
			{
				auto xy = Avo::Point<Avo::Pixels>{GET_X_LPARAM(p_data_b), GET_Y_LPARAM(p_data_b)};

				if (xy == m_mousePosition)
				{
					return 0;
				}

				Avo::MouseEvent mouseEvent;
				mouseEvent.xy = xy/m_dipToPixelFactor;
				mouseEvent.movement = Avo::Vector2d{(xy - m_mousePosition)/m_dipToPixelFactor};

				m_mousePosition = xy;

				m_gui->lockThreads();
				m_gui->handleGlobalMouseMove(mouseEvent);
				m_gui->unlockThreads();

				if (m_isMouseOutsideClientArea)
				{
					SetCursor(m_cursorHandle);

					// This is to make the window recieve WM_MOUSELEAVE.
					auto trackStructure = TRACKMOUSEEVENT{};
					trackStructure.dwFlags = TME_LEAVE;
					trackStructure.cbSize = sizeof(TRACKMOUSEEVENT);
					trackStructure.hwndTrack = m_windowHandle;
					TrackMouseEvent(&trackStructure);

					m_isMouseOutsideClientArea = false;
				}

				return 0;
			}
			case WM_NCMOUSELEAVE:
			case WM_MOUSELEAVE:
			{
				if (GetCapture() != m_windowHandle)
				{
					POINT winMousePosition;
					GetCursorPos(&winMousePosition);

					bool const isMouseOverWindow = WindowFromPoint(winMousePosition) == m_windowHandle;

					ScreenToClient(m_windowHandle, &winMousePosition);

					auto const mousePosition = Avo::Point<Avo::Pixels>{winMousePosition.x, winMousePosition.y};
					if (m_size.getIsContaining(mousePosition) && isMouseOverWindow)
					{
						/*
							If it's a WM_MOUSELEAVE message, then it has entered the nonclient area if the new mouse position still is inside the 
							window. If it's a WM_NCMOUSELEAVE message, then it has entered the client area. Note that both these cases would mean 
							that the window has the CustomBorder style flag set.
						*/
						m_isMouseOutsideClientArea = p_message == WM_MOUSELEAVE;
						return 0;
					}

					m_isMouseOutsideClientArea = true;

					Avo::MouseEvent mouseEvent;
					mouseEvent.xy = mousePosition/m_dipToPixelFactor;
					mouseEvent.movement = Avo::Vector2d{(mousePosition - m_mousePosition)/m_dipToPixelFactor};

					m_mousePosition = mousePosition;

					{
						auto const lock = m_gui->createThreadLock();
						m_gui->handleGlobalMouseMove(mouseEvent);
						m_gui->handleGlobalMouseLeave(mouseEvent);
					}
					m_cursorType = Avo::Cursor::Unknown;
				}
				return 0;
			}
			case WM_NCHITTEST:
			{
				if (getHasCustomBorder())
				{
					if (p_data_b)
					{
						auto mousePosition = POINT{GET_X_LPARAM(p_data_b), GET_Y_LPARAM(p_data_b)};
						ScreenToClient(m_windowHandle, &mousePosition);

						auto const area = m_gui->getWindowBorderAreaAtPosition(
							pixelsToDips(Avo::Point<Avo::Pixels>{mousePosition.x, mousePosition.y})
						);
						if (IsMaximized(m_windowHandle) && area != Avo::WindowBorderArea::Dragging && area != Avo::WindowBorderArea::None)
						{
							return HTCLIENT;
						}
						switch (area)
						{
							case Avo::WindowBorderArea::TopLeftResize:
								return HTTOPLEFT;
							case Avo::WindowBorderArea::TopResize:
								return HTTOP;
							case Avo::WindowBorderArea::TopRightResize:
								return HTTOPRIGHT;
							case Avo::WindowBorderArea::LeftResize:
								return HTLEFT;
							case Avo::WindowBorderArea::RightResize:
								return HTRIGHT;
							case Avo::WindowBorderArea::BottomLeftResize:
								return HTBOTTOMLEFT;
							case Avo::WindowBorderArea::BottomResize:
								return HTBOTTOM;
							case Avo::WindowBorderArea::BottomRightResize:
								return HTBOTTOMRIGHT;
							case Avo::WindowBorderArea::Dragging:
								return HTCAPTION;
							case Avo::WindowBorderArea::None:
								return HTCLIENT;
						}
					}
					return 0;
				}
				break;
			}
			case WM_DPICHANGED:
			{
				m_gui->getDrawingContext()->setDpi(static_cast<float>(HIWORD(p_data_a)));
				m_dipToPixelFactor = HIWORD(p_data_a)/static_cast<float>(USER_DEFAULT_SCREEN_DPI);
				auto newRectangle = reinterpret_cast<RECT*>(p_data_b);
				SetWindowPos(
					m_windowHandle, 0, 
					newRectangle->left, newRectangle->top, 
					newRectangle->right - newRectangle->left, newRectangle->bottom - newRectangle->top, 
					SWP_NOZORDER | SWP_NOACTIVATE
				);
				return 0;
			}
			case WM_GETMINMAXINFO:
			{
				auto minMaxInfo = reinterpret_cast<MINMAXINFO*>(p_data_b);

				auto rect = RECT{0, 0, m_minSize.x, m_minSize.y};
				AdjustWindowRect(&rect, m_styles, 0);
				if (m_minSize.x > 0U || m_minSize.y > 0U)
				{
					minMaxInfo->ptMinTrackSize.x = rect.right - rect.left;
					minMaxInfo->ptMinTrackSize.y = rect.bottom - rect.top;
				}

				rect = {0, 0, m_maxSize.x, m_maxSize.y};
				AdjustWindowRect(&rect, m_styles, 0);
				if (m_maxSize.x > 0U || m_maxSize.y > 0U)
				{
					minMaxInfo->ptMaxTrackSize.x = rect.right - rect.left;
					minMaxInfo->ptMaxTrackSize.y = rect.bottom - rect.top;
				}
				return 0;
			}
			case WM_MOVE:
			{
				RECT rect;
				GetWindowRect(m_windowHandle, &rect);
				m_position = {rect.left, rect.top};

				return 0;
			}
			case WM_MOUSEWHEEL:
			{
				auto mousePosition = POINT{GET_X_LPARAM(p_data_b), GET_Y_LPARAM(p_data_b)};				
				ScreenToClient(m_windowHandle, &mousePosition);

				Avo::MouseEvent mouseEvent;
				mouseEvent.xy = Avo::Point{mousePosition.x, mousePosition.y}/m_dipToPixelFactor;
				mouseEvent.scrollDelta = GET_WHEEL_DELTA_WPARAM(p_data_a)/120.f;
				mouseEvent.modifierKeys = convertWindowsKeyStateToModifierKeyFlags(GET_KEYSTATE_WPARAM(p_data_a));

				m_gui->lockThreads();
				m_gui->handleGlobalMouseScroll(mouseEvent);
				m_gui->unlockThreads();

				return 0;
			}
			case WM_LBUTTONDOWN:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseDown(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Left));
				m_gui->unlockThreads();

				SetCapture(m_windowHandle);

				return 0;
			}
			case WM_LBUTTONUP:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseUp(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Left, m_isDoubleClicking));
				m_gui->unlockThreads();

				m_isDoubleClicking = false;

				ReleaseCapture();

				return 0;
			}
			case WM_LBUTTONDBLCLK:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseDown(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Left, true));
				m_gui->unlockThreads();

				m_isDoubleClicking = true;

				return 0;
			}
			case WM_RBUTTONDOWN:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseDown(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Right));
				m_gui->unlockThreads();

				return 0;
			}
			case WM_RBUTTONUP:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseUp(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Right, m_isDoubleClicking));
				m_gui->unlockThreads();

				m_isDoubleClicking = false;

				return 0;
			}
			case WM_RBUTTONDBLCLK:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseDown(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Right, true));
				m_gui->unlockThreads();

				m_isDoubleClicking = true;
				
				return 0;
			}
			case WM_MBUTTONDOWN:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseDown(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Middle));
				m_gui->unlockThreads();

				return 0;
			}
			case WM_MBUTTONUP:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseUp(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Middle, m_isDoubleClicking));
				m_gui->unlockThreads();

				m_isDoubleClicking = false;

				return 0;
			}
			case WM_MBUTTONDBLCLK:
			{
				m_gui->lockThreads();
				m_gui->handleGlobalMouseDown(createMouseButtonEvent(p_data_a, p_data_b, Avo::MouseButton::Middle, true));
				m_gui->unlockThreads();

				m_isDoubleClicking = true;

				return 0;
			}
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				m_gui->lockThreads();
				m_gui->sendGlobalKeyboardKeyDownEvents({convertWindowsDataToKeyboardKey(p_data_a), getIsKeyRepeated(p_data_b)});
				m_gui->unlockThreads();

				return 0;
			}
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				m_gui->lockThreads();
				m_gui->sendGlobalKeyboardKeyUpEvents({convertWindowsDataToKeyboardKey(p_data_a)});
				m_gui->unlockThreads();

				return 0;
			}
			case WM_CHAR:
			{
				// Length is 5 because 4 is the max number of bytes in a utf-8 encoded character, and the null terminator is included
				char character[5];
				Avo::convertUtf16ToUtf8(reinterpret_cast<char16 const*>(&p_data_a), {character, 5});

				m_gui->lockThreads();
				m_gui->sendGlobalCharacterInputEvents({character, getIsKeyRepeated(p_data_b)});
				m_gui->unlockThreads();
				return 0;
			}
			case WM_MENUCHAR:
			{
				return 1 << 16;
			}
			case WM_CLOSE:
			{
				if (m_willClose)
				{
					DeleteColorSpace(GetColorSpace(GetDC(m_windowHandle)));
					
					m_oleDropTarget = nullptr;
					m_oleDropSource = nullptr;
					RevokeDragDrop(m_windowHandle);
					OleUninitialize();

					m_isOpen = false;
					DestroyWindow(m_windowHandle);
				}
				else
				{
					m_willClose = sendWindowCloseEvents({this, m_size/m_dipToPixelFactor});
				}

				return 0;
			}
			case WM_DESTROY:
			{
				m_windowHandle = 0;
				s_numberOfWindows--;
				if (!s_numberOfWindows)
				{
					UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandle(0));
				}
				PostQuitMessage(0);

				return 0;
			}
		}
		return ~0LL;
	}

	static auto CALLBACK handleGlobalEvents(
		HWND const p_windowHandle, UINT const p_message, 
		WPARAM const p_data_a, LPARAM const p_data_b
	) -> LRESULT
	{
		WindowsWindow* window;
		if (p_message == WM_CREATE)
		{
			window = static_cast<WindowsWindow*>(reinterpret_cast<CREATESTRUCT*>(p_data_b)->lpCreateParams);
			SetWindowLongPtr(p_windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			window->setNativeHandle(p_windowHandle);
		}
		else
		{
			window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(p_windowHandle, GWLP_USERDATA));
		}
		if (window)
		{
			if (auto result = window->handleEvent(p_message, p_data_a, p_data_b);
			    result != ~0LL)
			{
				return result;
			}
		}
		return DefWindowProc(p_windowHandle, p_message, p_data_a, p_data_b);
	}

	//------------------------------

private:
	Avo::Gui* m_gui;
public:
	WindowsWindow(Avo::Gui* p_gui) :
		m_gui{p_gui} 
	{
		setCursor(Avo::Cursor::Arrow);
	}
	// WindowsWindow(Avo::Gui* p_gui, std::string_view p_title, Avo::Size<> p_size,
	//               Avo::WindowStyleFlags p_styleFlags = Avo::WindowStyleFlags::Default, Avo::Window* p_parent = 0) :
	// 	m_gui(p_gui), 
	// 	m_crossPlatformStyles(p_styleFlags)
	// {
	// 	Window::create(p_title, p_size, p_styleFlags, p_parent);

	// 	setCursor(Avo::Cursor::Arrow);
	// }
	~WindowsWindow()
	{
		m_messageThread.join();
		DestroyCursor(m_cursorHandle);
	}
};
std::atomic<uint32> WindowsWindow::s_numberOfWindows;

#endif

#ifdef __linux__
/*
	Some resources that I used:
	https://www.x.org/docs/X11/xlib.pdf
	https://specifications.freedesktop.org/wm-spec/wm-spec-1.4.html
	https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html
	https://github.com/SFML/SFML/tree/master/src/SFML/Window/Unix
	The spec websites are really ugly and it's hard to read on them for me, a tip is to use a 
	"reader view" chrome extension or similar functionality in other browsers :) 
*/

constexpr float NORMAL_DPI = 96.f;

typedef Window XWindow;

class LinuxWindow : public Avo::Window
{
private:
	Avo::Gui* m_gui = nullptr;

	XWindow m_windowHandle = 0;

	XIM m_inputMethod = nullptr;
	XIC m_inputContext = nullptr;

	Atom m_windowManagerProtocolsMessageType{0};
	Atom m_windowCloseEvent{0};

	Atom m_backgroundColorMessageType{0};
	
	bool m_isOpen{false};
	Avo::Point<uint32> m_size;
	Avo::Point<> m_minSize;
	Avo::Point<> m_maxSize;

	Avo::WindowStyleFlags m_crossPlatformStyles{Avo::WindowStyleFlags::None};

	float m_dipToPixelFactor{1.f};

	//------------------------------

	KeySym convertKeyboardKeyToKeySym(Avo::KeyboardKey p_key) const
	{
		switch (p_key)
		{
			case Avo::KeyboardKey::Menu:
				return XK_Menu;
			case Avo::KeyboardKey::Backspace:
				return XK_BackSpace;
			case Avo::KeyboardKey::Clear:
				return XK_Clear;
			case Avo::KeyboardKey::Tab:
				return XK_Tab;
			case Avo::KeyboardKey::Return:
				return XK_Return;
			case Avo::KeyboardKey::Shift:
				return XK_Shift_L;
			case Avo::KeyboardKey::Control:
				return XK_Control_L;
			case Avo::KeyboardKey::Alt:
				return XK_Alt_L;
			case Avo::KeyboardKey::Pause:
				return XK_Pause;
			case Avo::KeyboardKey::CapsLock:
				return XK_Caps_Lock;
			case Avo::KeyboardKey::Escape:
				return XK_Escape;
			case Avo::KeyboardKey::Spacebar:
				return XK_space;
			case Avo::KeyboardKey::PageUp:
				return XK_Page_Up;
			case Avo::KeyboardKey::PageDown:
				return XK_Page_Down;
			case Avo::KeyboardKey::End:
				return XK_End;
			case Avo::KeyboardKey::Home:
				return XK_Home;
			case Avo::KeyboardKey::Left:
				return XK_Left;
			case Avo::KeyboardKey::Right:
				return XK_Right;
			case Avo::KeyboardKey::Up:
				return XK_Up;
			case Avo::KeyboardKey::Down:
				return XK_Down;
			case Avo::KeyboardKey::PrintScreen:
				return XK_Print;
			case Avo::KeyboardKey::Insert:
				return XK_Insert;
			case Avo::KeyboardKey::Delete:
				return XK_Delete;
			case Avo::KeyboardKey::Help:
				return XK_Help;
			case Avo::KeyboardKey::Numpad0:
				return XK_KP_0;
			case Avo::KeyboardKey::Numpad1:
				return XK_KP_1;
			case Avo::KeyboardKey::Numpad2:
				return XK_KP_2;
			case Avo::KeyboardKey::Numpad3:
				return XK_KP_3;
			case Avo::KeyboardKey::Numpad4:
				return XK_KP_4;
			case Avo::KeyboardKey::Numpad5:
				return XK_KP_5;
			case Avo::KeyboardKey::Numpad6:
				return XK_KP_6;
			case Avo::KeyboardKey::Numpad7:
				return XK_KP_7;
			case Avo::KeyboardKey::Numpad8:
				return XK_KP_8;
			case Avo::KeyboardKey::Numpad9:
				return XK_KP_9;
			case Avo::KeyboardKey::Add:
				return XK_KP_Add;
			case Avo::KeyboardKey::Subtract:
				return XK_KP_Subtract;
			case Avo::KeyboardKey::Multiply:
				return XK_KP_Multiply;
			case Avo::KeyboardKey::Divide:
				return XK_KP_Divide;
			case Avo::KeyboardKey::F1:
				return XK_F1;
			case Avo::KeyboardKey::F2:
				return XK_F2;
			case Avo::KeyboardKey::F3:
				return XK_F3;
			case Avo::KeyboardKey::F4:
				return XK_F4;
			case Avo::KeyboardKey::F5:
				return XK_F5;
			case Avo::KeyboardKey::F6:
				return XK_F6;
			case Avo::KeyboardKey::F7:
				return XK_F7;
			case Avo::KeyboardKey::F8:
				return XK_F8;
			case Avo::KeyboardKey::F9:
				return XK_F9;
			case Avo::KeyboardKey::F10:
				return XK_F10;
			case Avo::KeyboardKey::F11:
				return XK_F11;
			case Avo::KeyboardKey::F12:
				return XK_F12;
			case Avo::KeyboardKey::F13:
				return XK_F13;
			case Avo::KeyboardKey::F14:
				return XK_F14;
			case Avo::KeyboardKey::F15:
				return XK_F15;
			case Avo::KeyboardKey::F16:
				return XK_F16;
			case Avo::KeyboardKey::F17:
				return XK_F17;
			case Avo::KeyboardKey::F18:
				return XK_F18;
			case Avo::KeyboardKey::F19:
				return XK_F19;
			case Avo::KeyboardKey::F20:
				return XK_F20;
			case Avo::KeyboardKey::F21:
				return XK_F21;
			case Avo::KeyboardKey::F22:
				return XK_F22;
			case Avo::KeyboardKey::F23:
				return XK_F23;
			case Avo::KeyboardKey::F24:
				return XK_F24;
			case Avo::KeyboardKey::NumLock:
				return XK_Num_Lock;
			case Avo::KeyboardKey::Number0:
				return XK_0;
			case Avo::KeyboardKey::Number1:
				return XK_1;
			case Avo::KeyboardKey::Number2:
				return XK_2;
			case Avo::KeyboardKey::Number3:
				return XK_3;
			case Avo::KeyboardKey::Number4:
				return XK_4;
			case Avo::KeyboardKey::Number5:
				return XK_5;
			case Avo::KeyboardKey::Number6:
				return XK_6;
			case Avo::KeyboardKey::Number7:
				return XK_7;
			case Avo::KeyboardKey::Number8:
				return XK_8;
			case Avo::KeyboardKey::Number9:
				return XK_9;
			case Avo::KeyboardKey::A:
				return XK_A;
			case Avo::KeyboardKey::B:
				return XK_B;
			case Avo::KeyboardKey::C:
				return XK_C;
			case Avo::KeyboardKey::D:
				return XK_D;
			case Avo::KeyboardKey::E:
				return XK_E;
			case Avo::KeyboardKey::F:
				return XK_F;
			case Avo::KeyboardKey::G:
				return XK_G;
			case Avo::KeyboardKey::H:
				return XK_H;
			case Avo::KeyboardKey::I:
				return XK_I;
			case Avo::KeyboardKey::J:
				return XK_J;
			case Avo::KeyboardKey::K:
				return XK_K;
			case Avo::KeyboardKey::L:
				return XK_L;
			case Avo::KeyboardKey::M:
				return XK_M;
			case Avo::KeyboardKey::N:
				return XK_N;
			case Avo::KeyboardKey::O:
				return XK_O;
			case Avo::KeyboardKey::P:
				return XK_P;
			case Avo::KeyboardKey::Q:
				return XK_Q;
			case Avo::KeyboardKey::R:
				return XK_R;
			case Avo::KeyboardKey::S:
				return XK_S;
			case Avo::KeyboardKey::T:
				return XK_T;
			case Avo::KeyboardKey::U:
				return XK_U;
			case Avo::KeyboardKey::V:
				return XK_V;
			case Avo::KeyboardKey::W:
				return XK_W;
			case Avo::KeyboardKey::X:
				return XK_X;
			case Avo::KeyboardKey::Y:
				return XK_Y;
			case Avo::KeyboardKey::Z:
				return XK_Z;
			case Avo::KeyboardKey::Regional1:
				return XK_semicolon;
			case Avo::KeyboardKey::Regional2:
				return XK_slash;
			case Avo::KeyboardKey::Regional3:
				return XK_grave;
			case Avo::KeyboardKey::Regional4:
				return XK_bracketleft;
			case Avo::KeyboardKey::Regional5:
				return XK_backslash;
			case Avo::KeyboardKey::Regional6:
				return XK_bracketright;
			case Avo::KeyboardKey::Regional7:
				return XK_apostrophe;
		}
		return 0;
	}
	Avo::KeyboardKey convertKeySymToKeyboardKey(KeySym p_keySym) const
	{
		switch (p_keySym)
		{
			case XK_Menu:
				return Avo::KeyboardKey::Menu;
			case XK_BackSpace:
				return Avo::KeyboardKey::Backspace;
			case XK_Clear:
				return Avo::KeyboardKey::Clear;
			case XK_Tab:
				return Avo::KeyboardKey::Tab;
			case XK_Return:
				return Avo::KeyboardKey::Return;
			case XK_Shift_L:
			case XK_Shift_R:
				return Avo::KeyboardKey::Shift;
			case XK_Control_L:
			case XK_Control_R:
				return Avo::KeyboardKey::Control;
			case XK_Alt_L:
			case XK_Alt_R:
				return Avo::KeyboardKey::Alt;
			case XK_Pause:
				return Avo::KeyboardKey::Pause;
			case XK_Caps_Lock:
				return Avo::KeyboardKey::CapsLock;
			case XK_Escape:
				return Avo::KeyboardKey::Escape;
			case XK_space:
				return Avo::KeyboardKey::Spacebar;
			case XK_Page_Up:
				return Avo::KeyboardKey::PageUp;
			case XK_Page_Down:
				return Avo::KeyboardKey::PageDown;
			case XK_End:
				return Avo::KeyboardKey::End;
			case XK_Home:
				return Avo::KeyboardKey::Home;
			case XK_Left:
				return Avo::KeyboardKey::Left;
			case XK_Right:
				return Avo::KeyboardKey::Right;
			case XK_Up:
				return Avo::KeyboardKey::Up;
			case XK_Down:
				return Avo::KeyboardKey::Down;
			case XK_Print:
				return Avo::KeyboardKey::PrintScreen;
			case XK_Insert:
				return Avo::KeyboardKey::Insert;
			case XK_Delete:
				return Avo::KeyboardKey::Delete;
			case XK_Help:
				return Avo::KeyboardKey::Help;
			case XK_KP_0:
				return Avo::KeyboardKey::Numpad0;
			case XK_KP_1:
				return Avo::KeyboardKey::Numpad1;
			case XK_KP_2:
				return Avo::KeyboardKey::Numpad2;
			case XK_KP_3:
				return Avo::KeyboardKey::Numpad3;
			case XK_KP_4:
				return Avo::KeyboardKey::Numpad4;
			case XK_KP_5:
				return Avo::KeyboardKey::Numpad5;
			case XK_KP_6:
				return Avo::KeyboardKey::Numpad6;
			case XK_KP_7:
				return Avo::KeyboardKey::Numpad7;
			case XK_KP_8:
				return Avo::KeyboardKey::Numpad8;
			case XK_KP_9:
				return Avo::KeyboardKey::Numpad9;
			case XK_KP_Add:
				return Avo::KeyboardKey::Add;
			case XK_KP_Subtract:
				return Avo::KeyboardKey::Subtract;
			case XK_KP_Multiply:
				return Avo::KeyboardKey::Multiply;
			case XK_KP_Divide:
				return Avo::KeyboardKey::Divide;
			case XK_F1:
				return Avo::KeyboardKey::F1;
			case XK_F2:
				return Avo::KeyboardKey::F2;
			case XK_F3:
				return Avo::KeyboardKey::F3;
			case XK_F4:
				return Avo::KeyboardKey::F4;
			case XK_F5:
				return Avo::KeyboardKey::F5;
			case XK_F6:
				return Avo::KeyboardKey::F6;
			case XK_F7:
				return Avo::KeyboardKey::F7;
			case XK_F8:
				return Avo::KeyboardKey::F8;
			case XK_F9:
				return Avo::KeyboardKey::F9;
			case XK_F10:
				return Avo::KeyboardKey::F10;
			case XK_F11:
				return Avo::KeyboardKey::F11;
			case XK_F12:
				return Avo::KeyboardKey::F12;
			case XK_F13:
				return Avo::KeyboardKey::F13;
			case XK_F14:
				return Avo::KeyboardKey::F14;
			case XK_F15:
				return Avo::KeyboardKey::F15;
			case XK_F16:
				return Avo::KeyboardKey::F16;
			case XK_F17:
				return Avo::KeyboardKey::F17;
			case XK_F18:
				return Avo::KeyboardKey::F18;
			case XK_F19:
				return Avo::KeyboardKey::F19;
			case XK_F20:
				return Avo::KeyboardKey::F20;
			case XK_F21:
				return Avo::KeyboardKey::F21;
			case XK_F22:
				return Avo::KeyboardKey::F22;
			case XK_F23:
				return Avo::KeyboardKey::F23;
			case XK_F24:
				return Avo::KeyboardKey::F24;
			case XK_Num_Lock:
				return Avo::KeyboardKey::NumLock;
			case XK_0:
				return Avo::KeyboardKey::Number0;
			case XK_1:
				return Avo::KeyboardKey::Number1;
			case XK_2:
				return Avo::KeyboardKey::Number2;
			case XK_3:
				return Avo::KeyboardKey::Number3;
			case XK_4:
				return Avo::KeyboardKey::Number4;
			case XK_5:
				return Avo::KeyboardKey::Number5;
			case XK_6:
				return Avo::KeyboardKey::Number6;
			case XK_7:
				return Avo::KeyboardKey::Number7;
			case XK_8:
				return Avo::KeyboardKey::Number8;
			case XK_9:
				return Avo::KeyboardKey::Number9;
			case XK_A:
				return Avo::KeyboardKey::A;
			case XK_B:
				return Avo::KeyboardKey::B;
			case XK_C:
				return Avo::KeyboardKey::C;
			case XK_D:
				return Avo::KeyboardKey::D;
			case XK_E:
				return Avo::KeyboardKey::E;
			case XK_F:
				return Avo::KeyboardKey::F;
			case XK_G:
				return Avo::KeyboardKey::G;
			case XK_H:
				return Avo::KeyboardKey::H;
			case XK_I:
				return Avo::KeyboardKey::I;
			case XK_J:
				return Avo::KeyboardKey::J;
			case XK_K:
				return Avo::KeyboardKey::K;
			case XK_L:
				return Avo::KeyboardKey::L;
			case XK_M:
				return Avo::KeyboardKey::M;
			case XK_N:
				return Avo::KeyboardKey::N;
			case XK_O:
				return Avo::KeyboardKey::O;
			case XK_P:
				return Avo::KeyboardKey::P;
			case XK_Q:
				return Avo::KeyboardKey::Q;
			case XK_R:
				return Avo::KeyboardKey::R;
			case XK_S:
				return Avo::KeyboardKey::S;
			case XK_T:
				return Avo::KeyboardKey::T;
			case XK_U:
				return Avo::KeyboardKey::U;
			case XK_V:
				return Avo::KeyboardKey::V;
			case XK_W:
				return Avo::KeyboardKey::W;
			case XK_X:
				return Avo::KeyboardKey::X;
			case XK_Y:
				return Avo::KeyboardKey::Y;
			case XK_Z:
				return Avo::KeyboardKey::Z;
			case XK_semicolon:
				return Avo::KeyboardKey::Regional1;
			case XK_slash:
				return Avo::KeyboardKey::Regional2;
			case XK_grave:
				return Avo::KeyboardKey::Regional3;
			case XK_bracketleft:
				return Avo::KeyboardKey::Regional4;
			case XK_backslash:
				return Avo::KeyboardKey::Regional5;
			case XK_bracketright:
				return Avo::KeyboardKey::Regional6;
			case XK_apostrophe:
				return Avo::KeyboardKey::Regional7;
		}
		return Avo::KeyboardKey::None;
	}
	
	//------------------------------

	bool m_hasCreatedWindow = false;
	std::condition_variable m_hasCreatedWindowConditionVariable;
	std::mutex m_hasCreatedWindowMutex;
	std::thread m_messageThread;
	void thread_runEventLoop(std::string_view p_title, float p_x, float p_y, float p_width, float p_height, Avo::Window* p_parent)
	{
		XInitThreads();

		m_server = XOpenDisplay(nullptr); // Open connection to server

		auto displayWidth = (float)XDisplayWidth(m_server, 0);
		auto displayHeight = (float)XDisplayHeight(m_server, 0);
		m_dipToPixelFactor = displayWidth/(float)XDisplayWidthMM(m_server, 0)*25.4f/NORMAL_DPI;
		m_size.set(p_width*m_dipToPixelFactor, p_height*m_dipToPixelFactor);

		//------------------------------
		// Select the OpenGL visual to be used when creating the window.

		int framebufferAttributes[] =
		{
			GLX_X_RENDERABLE, 1,
			GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
			GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8,
			GLX_DEPTH_SIZE, 0, // 2D graphics, no z-buffering.
			GLX_STENCIL_SIZE, 0,
			GLX_DOUBLEBUFFER, 1,
			0 // Null terminator
		};

		int numberOfMatchingConfigurations = 0;
		GLXFBConfig* framebufferConfigurations = glXChooseFBConfig(m_server, DefaultScreen(m_server), framebufferAttributes, &numberOfMatchingConfigurations);

		XVisualInfo* visualInfo = glXGetVisualFromFBConfig(m_server, *framebufferConfigurations);

		XFree(framebufferConfigurations);

		//------------------------------
		// Create window

		m_colormap = XCreateColormap(m_server, RootWindow(m_server, visualInfo->screen), visualInfo->visual, 0);
		XSetWindowAttributes windowAttributes = {};
		windowAttributes.colormap = m_colormap;
		windowAttributes.event_mask =
			ExposureMask |
			EnterWindowMask | LeaveWindowMask |
			StructureNotifyMask |
			PointerMotionMask |
			ButtonPressMask | ButtonReleaseMask |
			ButtonMotionMask |
			KeyPressMask | KeyReleaseMask;

		m_windowHandle = XCreateWindow(
			m_server, p_parent ? (XWindow)p_parent->getNativeHandle() : RootWindow(m_server, visualInfo->screen),
			0, 0, // Initial x and y are ignored by the window manager
			(unsigned int)(p_width*m_dipToPixelFactor),
			(unsigned int)(p_height*m_dipToPixelFactor),
			0,
			visualInfo->depth,
			InputOutput,
			visualInfo->visual,
			CWEventMask | CWBorderPixel | CWColormap,
			&windowAttributes
		);

		XFree(visualInfo);

		setTitle(p_title);
		XMapWindow(m_server, m_windowHandle); // Show the window
		setPosition(p_x*(displayWidth - p_width*m_dipToPixelFactor), p_y*(displayHeight - p_height*m_dipToPixelFactor));

		//------------------------------
		// Open keyboard input

		m_inputMethod = XOpenIM(m_server, nullptr, nullptr, nullptr);
		m_inputContext = XCreateIC(
			m_inputMethod,
			XNInputStyle, XIMPreeditNothing | XIMStatusNothing, // Input style flags
			XNClientWindow, m_windowHandle,
			XNFocusWindow, m_windowHandle,
			nullptr // Null terminator
		);

		//------------------------------

		m_backgroundColorMessageType = XInternAtom(m_server, "AVOGUI_SET_BACKGROUND_COLOR", false);

		// We want the window manager to tell us when the window should be closed.
		// WM_PROTOCOLS is the atom used to identify messages sent from the window manager in a ClientMessage.
		m_windowManagerProtocolsMessageType = XInternAtom(m_server, "WM_PROTOCOLS", true);
		// This is the atom sent as the data in a ClientMessage with type WM_PROTOCOLS, to indicate the close event.
		m_windowCloseEvent = XInternAtom(m_server, "WM_DELETE_WINDOW", 0);
		// Tell the window manager that we want it to send the event through WM_PROTOCOLS.
		XSetWMProtocols(m_server, m_windowHandle, &m_windowCloseEvent, 1);

		XFlush(m_server); // Execute the command queue

		//------------------------------

		XEvent event;
		Time lastKeyPressTime = 0;
		KeyCode lastKeyPressKeyCode = 0;

		m_isOpen = true;
		while (m_isOpen)
		{
			XNextEvent(m_server, &event);

			if (XFilterEvent(&event, m_windowHandle))
			{
				continue;
			}

			switch (event.type)
			{
				case Expose:
				{
					if (!event.xexpose.count)
					{
						m_gui->invalidateRectangle(
							(float)event.xexpose.x/m_dipToPixelFactor, (float)event.xexpose.y/m_dipToPixelFactor,
							(float)event.xexpose.width/m_dipToPixelFactor, (float)event.xexpose.height/m_dipToPixelFactor
						);
					}
					break;
				}
				case ClientMessage:
				{
					if (event.xclient.message_type == m_windowManagerProtocolsMessageType)
					{
						// Sent from the window manager when the user has tried to close the window,
						// it is up to us to decide whether to actually close and exit the application.
						if (event.xclient.data.l[0] == m_windowCloseEvent)
						{
							if (m_willClose)
							{
								m_isOpen = false;
								XDestroyWindow(m_server, m_windowHandle);
							}
							else
							{
								m_willClose = sendWindowCloseEvents({this, m_size.x/m_dipToPixelFactor, m_size.y/m_dipToPixelFactor});
							}
						}
					}
					else if (event.xclient.message_type == m_backgroundColorMessageType)
					{
						// Sent from drawing context
						XColor xColor;
						xColor.red = event.xclient.data.l[0];
						xColor.green = event.xclient.data.l[1];
						xColor.blue = event.xclient.data.l[2];
						xColor.flags = DoRed | DoGreen | DoBlue;
						XAllocColor(m_server, m_colormap, &xColor);
						XSetWindowBackground(m_server, m_windowHandle, xColor.pixel);
					}
					break;
				}
				case MotionNotify:
				{

					break;
				}
				case GravityNotify:
				{
					break;
				}
				case ConfigureNotify:
				{
					if (!m_hasCreatedWindow)
					{
						Avo::WindowEvent windowEvent = {this, p_width, p_height};
						windowCreateListeners(windowEvent);

						windowSizeChangeListeners(windowEvent);

						m_hasCreatedWindowMutex.lock();
						m_hasCreatedWindow = true;
						m_hasCreatedWindowMutex.unlock();
						m_hasCreatedWindowConditionVariable.notify_one();
					}
					else if (m_size.x != event.xconfigure.width || m_size.y != event.xconfigure.height)
					{
						m_size.set(event.xconfigure.width, event.xconfigure.height);
						windowSizeChangeListeners(Avo::WindowEvent{this, m_size.x/m_dipToPixelFactor, m_size.y/m_dipToPixelFactor});
					}
					break;
				}
				case ButtonPress:
				{
					break;
				}
				case ButtonRelease:
				{

					break;
				}
				case KeyPress:
				{
					// Length is 5 because 4 is the max number of bytes in a utf-8 encoded character
					char character[5];
					KeySym key;
					Status characterLookupStatus;
					int length = Xutf8LookupString(m_inputContext, &event.xkey, character, 4, &key, &characterLookupStatus);

					Avo::KeyboardEvent keyboardEvent;
					keyboardEvent.isRepeated = lastKeyPressKeyCode == event.xkey.keycode && event.xkey.time < lastKeyPressTime + 2;
					if (characterLookupStatus == XLookupBoth || characterLookupStatus == XLookupChars)
					{
						keyboardEvent.character = character;
						m_gui->lockThreads();
						m_gui->sendGlobalCharacterInputEvents(keyboardEvent);
						m_gui->unlockThreads();
					}
					if (characterLookupStatus == XLookupBoth || characterLookupStatus == XLookupKeySym)
					{
						keyboardEvent.character = "";
						keyboardEvent.key = convertKeySymToKeyboardKey(key);
						m_gui->lockThreads();
						m_gui->sendGlobalKeyboardKeyDownEvents(keyboardEvent);
						m_gui->unlockThreads();
					}

					lastKeyPressTime = event.xkey.time;

					break;
				}
				case KeyRelease:
				{
					Avo::KeyboardEvent keyboardEvent;

					// Try the four modifier groups until one matches
					for (uint32 a = 0; a < 4; a++)
					{
						if (Avo::KeyboardKey::None != (keyboardEvent.key = convertKeySymToKeyboardKey(XLookupKeysym(&event.xkey, a))))
						{
							break;
						}
					}

					if (keyboardEvent.key != Avo::KeyboardKey::None)
					{
						keyboardEvent.isRepeated = lastKeyPressKeyCode == event.xkey.keycode && event.xkey.time < lastKeyPressTime + 2;
						m_gui->lockThreads();
						m_gui->sendGlobalKeyboardKeyUpEvents(keyboardEvent);
						m_gui->unlockThreads();
						lastKeyPressTime = event.xkey.time;
					}

					break;
				}
			} 
		}
		XDestroyIC(m_inputContext);
		XCloseIM(m_inputMethod);
		XFreeColormap(m_server, m_colormap);
		XCloseDisplay(m_server);
	}

private:
	Display* m_server = nullptr;
public:
	Display* getServer()
	{
		return m_server;
	}

private:
	Colormap m_colormap = 0;
public:
	Colormap getColormap()
	{
		return m_colormap;
	}
	Atom getBackgroundColorMessageType()
	{
		return m_backgroundColorMessageType;
	}

	void create(std::string_view p_title, float p_x, float p_y, float p_width, float p_height, Avo::WindowStyleFlags p_styleFlags, Avo::Window* p_parent) override
	{
		m_crossPlatformStyles = p_styleFlags;
		m_messageThread = std::thread(&LinuxWindow::thread_runEventLoop, this, p_title, p_x, p_y, p_width, p_height, p_parent);

		if (!m_hasCreatedWindow)
		{
			std::unique_lock<std::mutex> lock(m_hasCreatedWindowMutex);
			m_hasCreatedWindowConditionVariable.wait(lock, [=] { return (bool)m_hasCreatedWindow; });
		}
	}
	void create(std::string_view p_title, float p_width, float p_height, Avo::WindowStyleFlags p_styleFlags = Avo::WindowStyleFlags::Default, Avo::Window* p_parent = nullptr) override
	{
		create(p_title, 0.5f, 0.5f, p_width, p_height, p_styleFlags, p_parent);
	}

	void close() override 
	{
		if (m_windowHandle)
		{
			XEvent event;
			event.type = ClientMessage;
			event.xclient.message_type = m_windowManagerProtocolsMessageType;
			event.xclient.data.l[0] = m_windowCloseEvent;
			event.xclient.display = m_server;
			event.xclient.window = m_windowHandle;
			event.xclient.format = 32;
			event.xclient.send_event = true;
			XSendEvent(m_server, m_windowHandle, false, 0, &event);
			XFlush(m_server);
		}
	}
	bool getIsOpen() const override
	{
		return m_isOpen;
	}

	//------------------------------

	void enableUserInteraction() override 
	{
	}
	void disableUserInteraction() override 
	{
	}
	bool getIsUserInteractionEnabled() override 
	{
	}

	//------------------------------

	void setTitle(std::string_view p_title) override 
	{
		XTextProperty textProperty;
		#ifdef X_HAVE_UTF8_STRING
		textProperty.encoding = XInternAtom(m_server, "UTF8_STRING", 0);
		#else
		textProperty.encoding = XA_STRING;
		#endif
		textProperty.format = 8;
		textProperty.nitems = p_title.size();
		textProperty.value = (unsigned char*)p_title.data();
		XSetWMName(m_server, m_windowHandle, &textProperty);
		XSetWMIconName(m_server, m_windowHandle, &textProperty);
		XFlush(m_server);
	}
	std::string getTitle() const override
	{
		XTextProperty textProperty;
		XGetWMName(m_server, m_windowHandle, &textProperty);
		return (char const*)textProperty.value;
	}

	//------------------------------

	void setStyles(Avo::WindowStyleFlags p_styles) override 
	{
	}
	Avo::WindowStyleFlags getStyles() const override
	{
	}

	//------------------------------

	void* getNativeHandle() const override
	{
		return (void*)m_windowHandle;
	}

	//------------------------------

	void setIsFullscreen(bool p_isFullscreen) override 
	{
	}
	void switchFullscreen() override 
	{
	}
	bool getIsFullscreen() const override
	{
	}

	//------------------------------

	void hide() override 
	{
	}
	void show() override 
	{
	}

	void maximize() override 
	{
	}
	void minimize() override 
	{
	}
	void restore() override 
	{
	}

	void setState(Avo::WindowState p_state) override 
	{
	}
	Avo::WindowState getState() const override
	{
	}

	//------------------------------

	void setPosition(int32 p_x, int32 p_y) override 
	{
		if (m_windowHandle)
		{
			XMoveWindow(m_server, m_windowHandle, p_x, p_y);
			XFlush(m_server);
		}
	}
	Avo::Point<int32> getPosition() const override
	{
		Avo::Point<int32> result;
		XWindow child;
		XTranslateCoordinates(m_server, m_windowHandle, DefaultRootWindow(m_server), 0, 0, &result.x, &result.y, &child);
		return result;
	}
	int32 getPositionX() const override
	{
		int32 x;
		int32 y;
		XWindow child;
		XTranslateCoordinates(m_server, m_windowHandle, DefaultRootWindow(m_server), 0, 0, &x, &y, &child);
		return x;
	}
	int32 getPositionY() const override
	{
		int32 x;
		int32 y;
		XWindow child;
		XTranslateCoordinates(m_server, m_windowHandle, DefaultRootWindow(m_server), 0, 0, &x, &y, &child);
		return y;
	}

	void setSize(float p_width, float p_height) override 
	{
		if (m_windowHandle)
		{
			XResizeWindow(m_server, m_windowHandle, p_width*m_dipToPixelFactor, p_height*m_dipToPixelFactor);
			XFlush(m_server);
		}
	}
	Avo::Point<> getSize() const override
	{
		return m_size/m_dipToPixelFactor;
	}
	float getWidth() const override
	{
		return m_size.x/m_dipToPixelFactor;
	}
	float getHeight() const override
	{
		return m_size.y/m_dipToPixelFactor;
	}

	void setMinSize(float p_minWidth, float p_minHeight) override 
	{
		XSizeHints sizeHints = {};
		sizeHints.flags = PMinSize;
		sizeHints.min_width = p_minWidth*m_dipToPixelFactor;
		sizeHints.min_height = p_minHeight*m_dipToPixelFactor; 
		XSetWMNormalHints(m_server, m_windowHandle, &sizeHints);
		m_minSize.set(p_minWidth, p_minHeight);
	}
	Avo::Point<> getMinSize() const override
	{
		return m_minSize;
	}
	float getMinWidth() const override
	{
		return m_minSize.x;
	}
	float getMinHeight() const override
	{
		return m_minSize.y;
	}

	void setMaxSize(float p_maxWidth, float p_maxHeight) override 
	{
		XSizeHints sizeHints = {};
		sizeHints.flags = PMaxSize;
		sizeHints.max_width = p_maxWidth*m_dipToPixelFactor;
		sizeHints.max_height = p_maxHeight*m_dipToPixelFactor;
		XSetWMNormalHints(m_server, m_windowHandle, &sizeHints);
		m_maxSize.set(p_maxWidth, p_maxHeight);
	}
	Avo::Point<> getMaxSize() const override
	{
		return m_maxSize;
	}
	float getMaxWidth() const override
	{
		return m_maxSize.x;
	}
	float getMaxHeight() const override
	{
		return m_maxSize.y;
	}

	//------------------------------

	Avo::Rectangle<uint32> getMonitorBounds() const override
	{
	}
	Avo::Point<uint32> getMonitorPosition() const override
	{
	}
	Avo::Point<uint32> getMonitorSize() const override
	{
	}
	uint32 getMonitorWidth() const override
	{
	}
	uint32 getMonitorHeight() const override
	{
	}

	//------------------------------

	Avo::Rectangle<uint32> getWorkAreaBounds() const override
	{
	}
	Avo::Point<uint32> getWorkAreaPosition() const override
	{
	}
	Avo::Point<uint32> getWorkAreaSize() const override
	{
	}
	uint32 getWorkAreaWidth() const override
	{
	}
	uint32 getWorkAreaHeight() const override
	{
	}

	//------------------------------

	bool getIsKeyDown(Avo::KeyboardKey p_key) const override
	{
		KeySym keySym = convertKeyboardKeyToKeySym(p_key);
		KeyCode keyCode = XKeysymToKeycode(m_server, keySym);
		
		char keymap[32];
		XQueryKeymap(m_server, keymap);
		if (keyCode)
		{
			bool result = keymap[keyCode/8] & 1 << keyCode % 8; // Look up operator precedence ;)
			if (!result && keySym == XK_Control_L)
			{
				return keymap[XKeysymToKeycode(m_server, XK_Control_R)/8] & 1 << keyCode % 8;
			}
			if (!result && keySym == XK_Shift_L)
			{
				return keymap[XKeysymToKeycode(m_server, XK_Shift_R)/8] & 1 << keyCode % 8;
			}
			if (!result && keySym == XK_Alt_L)
			{
				return keymap[XKeysymToKeycode(m_server, XK_Alt_R)/8] & 1 << keyCode % 8;
			}
			return result;
		}
		return false;
	}
	bool getIsMouseButtonDown(Avo::MouseButton p_button) const override
	{
	}
	Avo::Point<> getMousePosition() const override
	{
	}

	//------------------------------

	void setCursor(Avo::Cursor p_cursor) override 
	{
		
	}
	Avo::Cursor getCursor() const override
	{
	}

	//------------------------------

	float getDipToPixelFactor() const override
	{
		return m_dipToPixelFactor;
	}

	//------------------------------

	Avo::DragDropOperation dragAndDropString(std::string_view p_string, Avo::Image const& p_dragImage,
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData = 0u) override
	{
	}
	Avo::DragDropOperation dragAndDropString(std::u16string_view p_string, Avo::Image const& p_dragImage,
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData = 0u) override
	{
	}

	Avo::DragDropOperation dragAndDropImage(Avo::Image const& p_image, Avo::Image const& p_dragImage,
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}

	Avo::DragDropOperation dragAndDropFile(uint8 const* p_data, uint32 p_dataSize, std::string_view p_name,
		Avo::Image const& p_dragImage, Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}
	Avo::DragDropOperation dragAndDropFile(uint8 const* p_data, uint32 p_dataSize, std::u16string_view p_name,
		Avo::Image const& p_dragImage, Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}
	Avo::DragDropOperation dragAndDropFile(std::vector<uint8> const& p_data, std::string_view p_name,
		Avo::Image const& p_dragImage, Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}
	Avo::DragDropOperation dragAndDropFile(std::vector<uint8> const& p_data, std::u16string_view p_name, 
		Avo::Image const& p_dragImage, Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}
	Avo::DragDropOperation dragAndDropFile(std::string_view p_path, Avo::Image const& p_dragImage, 
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}
	Avo::DragDropOperation dragAndDropFile(std::u16string_view p_path, Avo::Image const& p_dragImage, 
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}

	Avo::DragDropOperation dragAndDropFiles(std::vector<std::string> const& p_paths, Avo::Image const& p_dragImage, 
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData = 0u) override
	{
	}
	Avo::DragDropOperation dragAndDropFiles(std::vector<std::u16string> const& p_paths, Avo::Image const& p_dragImage, 
		Avo::Point<> p_dragImageCursorPosition, uint64 p_additionalData) override
	{
	}

	//------------------------------

	void setClipboardString(std::u16string_view p_string, uint64 p_additionalData) const override
	{
	}
	void setClipboardString(std::string_view p_string, uint64 p_additionalData) const override
	{
	}

	void setClipboardImage(Avo::Image const& p_image, uint64 p_additionalData) const override
	{
	}

	void setClipboardFile(uint8 const* p_data, uint32 p_dataSize, std::string_view p_name, uint64 p_additionalData) const override
	{
	}
	void setClipboardFile(uint8 const* p_data, uint32 p_dataSize, std::u16string_view p_name, uint64 p_additionalData) const override
	{
	}
	void setClipboardFile(std::vector<uint8> const& p_data, std::string_view p_name, uint64 p_additionalData) const override
	{
	}
	void setClipboardFile(std::vector<uint8> const& p_data, std::u16string_view p_name, uint64 p_additionalData) const override
	{
	}
	void setClipboardFile(std::string_view p_path, uint64 p_additionalData) const override
	{
	}
	void setClipboardFile(std::u16string_view p_path, uint64 p_additionalData) const override
	{
	}

	void setClipboardFiles(std::vector<std::string> const& p_paths, uint64 p_additionalData) const override
	{
	}
	void setClipboardFiles(std::vector<std::u16string> const& p_paths, uint64 p_additionalData) const override
	{
	}

	std::unique_ptr<Avo::ClipboardData> getClipboardData() const override
	{
		return nullptr;
	}

	LinuxWindow(Avo::Gui* p_gui) :
		m_gui{p_gui}
	{
	}
	~LinuxWindow()
	{
		if (m_messageThread.joinable())
		{
			m_messageThread.join();
		}
	}
};
#endif

#ifdef _WIN32
auto convertRectangleToRectf(Avo::Rectangle<> const p_rectangle) -> D2D1_RECT_F
{
	return {p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom};
}
auto convertSizefToSize(D2D1_SIZE_F const p_size) -> Avo::Size<>
{
	return {p_size.width, p_size.height};
}
auto convertSizefToSize(Avo::Size<> const p_size) -> D2D1_SIZE_F
{
	return {p_size.width, p_size.height};
}


class Direct2dImage : public Avo::Image
{
private:
	Avo::Rectangle<> m_cropRectangle;
public:
	auto setCropRectangle(Avo::Rectangle<> p_rectangle) -> void override
	{
		m_cropRectangle = p_rectangle;
	}
	auto getCropRectangle() const -> Avo::Rectangle<> override
	{
		return m_cropRectangle;
	}

	//------------------------------

private:
	Avo::ImageBoundsSizing m_boundsSizing = Avo::ImageBoundsSizing::Stretch;
public:
	auto setBoundsSizing(Avo::ImageBoundsSizing p_sizeMode) -> void override
	{
		m_boundsSizing = p_sizeMode;
	}
	auto getBoundsSizing() const -> Avo::ImageBoundsSizing override
	{
		return m_boundsSizing;
	}

private:
	Avo::Point<> m_boundsPositioning{0.5f, 0.5f};
public:
	auto setBoundsPositioning(Avo::Point<Avo::Factor> p_position) -> void override
	{
		m_boundsPositioning = p_position;
	}
	auto setBoundsPositioningX(Avo::Factor p_x) -> void override
	{
		m_boundsPositioning.x = p_x;
	}
	auto setBoundsPositioningY(Avo::Factor p_y) -> void override
	{
		m_boundsPositioning.y = p_y;
	}
	auto getBoundsPositioning() const -> Avo::Point<Avo::Factor> override
	{
		return m_boundsPositioning;
	}
	auto getBoundsPositioningX() const -> float override
	{
		return m_boundsPositioning.x;
	}
	auto getBoundsPositioningY() const -> float override
	{
		return m_boundsPositioning.x;
	}

	//------------------------------

private:
	Avo::ImageScalingMethod m_scalingMethod = Avo::ImageScalingMethod::Smooth;
public:
	auto setScalingMethod(Avo::ImageScalingMethod p_scalingMethod) -> void override
	{
		m_scalingMethod = p_scalingMethod;
	}
	auto getScalingMethod() const -> Avo::ImageScalingMethod override
	{
		return m_scalingMethod;
	}

	//------------------------------

private:
	float m_opacity = 1.f;
public:
	auto setOpacity(Avo::Factor p_opacity) -> void override
	{
		m_opacity = p_opacity;
	}
	auto getOpacity() const -> Avo::Factor override
	{
		return m_opacity;
	}

	//------------------------------

private:
	ComReference<ID2D1Bitmap1> m_image;
public:
	auto getD2dBitmap() const -> ID2D1Bitmap1*
	{
		return m_image;
	}

	auto getOriginalSize() const -> Avo::Size<> override
	{
		auto const size = m_image->GetSize();
		return {size.width, size.height};
	}
	auto getOriginalWidth() const -> Avo::Dip override
	{
		return m_image->GetSize().width;
	}
	auto getOriginalHeight() const -> Avo::Dip override
	{
		return m_image->GetSize().height;
	}

	auto getOriginalPixelSize() const -> Avo::Size<Avo::Pixels> override
	{
		auto const size = m_image->GetPixelSize();
		return {static_cast<Avo::Pixels>(size.width), static_cast<Avo::Pixels>(size.height)};
	}
	auto getOriginalPixelWidth() const -> Avo::Pixels override
	{
		return m_image->GetPixelSize().width;
	}
	auto getOriginalPixelHeight() const -> Avo::Pixels override
	{
		return m_image->GetPixelSize().height;
	}

	auto getInnerWidth() const -> float override
	{
		if (m_boundsSizing != Avo::ImageBoundsSizing::Stretch &&
		    m_boundsSizing == Avo::ImageBoundsSizing::Contain != 
		    m_bounds.getWidth()/m_bounds.getHeight() < m_image->GetSize().width/m_image->GetSize().height)
		{
			return m_bounds.getHeight()*m_image->GetSize().width/m_image->GetSize().height;
		}
		return m_bounds.getWidth();
	}
	auto getInnerHeight() const -> float override
	{
		if (m_boundsSizing != Avo::ImageBoundsSizing::Stretch &&
		    m_boundsSizing == Avo::ImageBoundsSizing::Contain != 
		    m_bounds.getWidth()/m_bounds.getHeight() > m_image->GetSize().width/m_image->GetSize().height)
		{
			return m_bounds.getWidth()*m_image->GetSize().height/m_image->GetSize().width;
		}
		return m_bounds.getHeight();
	}
	auto getInnerSize() const -> Avo::Point<> override
	{
		return {getInnerWidth(), getInnerHeight()};
	}
	auto getInnerBounds() const -> Avo::Rectangle<> override
	{
		if (m_boundsSizing == Avo::ImageBoundsSizing::Stretch)
		{
			return m_bounds;
		}

		auto innerBounds = m_bounds;

		auto const areBoundsProportionallyWider = m_bounds.getWidth()/m_bounds.getHeight() > m_image->GetSize().width/m_image->GetSize().height;
		if (m_boundsSizing == Avo::ImageBoundsSizing::Fill != areBoundsProportionallyWider)
		{
			innerBounds.setWidth(m_bounds.getHeight()*m_image->GetSize().width/m_image->GetSize().height);
		}
		else if (m_boundsSizing == Avo::ImageBoundsSizing::Contain != areBoundsProportionallyWider)
		{
			innerBounds.setHeight(m_bounds.getWidth()*m_image->GetSize().height/m_image->GetSize().width);
		}

		return innerBounds += Avo::Vector2d{m_boundsPositioning*(m_bounds.getSize() - innerBounds.getSize())};
	}

	//------------------------------

	Direct2dImage(ID2D1Bitmap1* p_image) :
		m_image{p_image},
		m_cropRectangle{0.f, 0.f, p_image->GetSize().width, p_image->GetSize().height}
	{
		m_bounds = m_cropRectangle;
	}
};

//------------------------------

class DirectWriteText : public Avo::Text
{
private:
	auto convertTextRange(TextRange const& p_range) -> DWRITE_TEXT_RANGE
	{
		return DWRITE_TEXT_RANGE{
			/*.startPosition =*/static_cast<UINT32>(p_range.length > 0 ? p_range.startPosition : 
		    	std::max(static_cast<Index>(0), p_range.startPosition - p_range.length)),
			/*.length =*/static_cast<UINT32>(p_range.length > 0 ? p_range.length : (p_range.length == 0 ? m_string.size() : -p_range.length))
		};
	}

protected:
	auto handleProtectedRectangleChange(Avo::Rectangle<> p_old) -> void override
	{
		auto const size = m_bounds.getSize();
		if (size.x != p_old.getWidth())
		{
			m_handle->SetMaxWidth(size.x);
		}
		if (size.y != p_old.getHeight())
		{
			m_handle->SetMaxHeight(size.y);
		}
	}

public:
	auto setWordWrapping(Avo::WordWrapping p_wordWrapping) -> void override
	{
		switch (p_wordWrapping)
		{
			case Avo::WordWrapping::Always:
				m_handle->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
				break;
			case Avo::WordWrapping::Emergency:
				m_handle->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
				break;
			case Avo::WordWrapping::Never:
				m_handle->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
				break;
			case Avo::WordWrapping::WholeWord:
				m_handle->SetWordWrapping(DWRITE_WORD_WRAPPING_WHOLE_WORD);
		}
	}
	auto getWordWrapping() const -> Avo::WordWrapping override
	{
		switch (m_handle->GetWordWrapping())
		{
			case DWRITE_WORD_WRAPPING_WRAP:
				return Avo::WordWrapping::Always;
			case DWRITE_WORD_WRAPPING_EMERGENCY_BREAK:
				return Avo::WordWrapping::Emergency;
			case DWRITE_WORD_WRAPPING_NO_WRAP:
				return Avo::WordWrapping::Never;
			case DWRITE_WORD_WRAPPING_WHOLE_WORD:
				return Avo::WordWrapping::WholeWord;
		}
		return Avo::WordWrapping::Unknown;
	}

	auto fitSizeToText() -> void override
	{
		DWRITE_TEXT_METRICS metrics;
		m_handle->GetMetrics(&metrics);

		DWRITE_OVERHANG_METRICS overhangMetrics;
		m_handle->GetOverhangMetrics(&overhangMetrics);

		ProtectedRectangle::setSize({metrics.width, m_handle->GetMaxHeight() + overhangMetrics.bottom + m_isTopTrimmed*overhangMetrics.top});
	}
	auto fitWidthToText() -> void override
	{
		DWRITE_TEXT_METRICS metrics;
		m_handle->GetMetrics(&metrics);
		ProtectedRectangle::setWidth(metrics.width);
	}
	auto fitHeightToText() -> void override
	{
		DWRITE_OVERHANG_METRICS overhangMetrics;
		m_handle->GetOverhangMetrics(&overhangMetrics);

		ProtectedRectangle::setHeight(m_handle->GetMaxHeight() + overhangMetrics.bottom + m_isTopTrimmed*overhangMetrics.top);
	}
	auto getMinimumSize() const -> Avo::Point<> override
	{
		DWRITE_TEXT_METRICS metrics;
		m_handle->GetMetrics(&metrics);

		DWRITE_OVERHANG_METRICS overhangMetrics;
		m_handle->GetOverhangMetrics(&overhangMetrics);

		return {metrics.width, m_handle->GetMaxHeight() + overhangMetrics.bottom + m_isTopTrimmed*overhangMetrics.top};
	}
	auto getMinimumWidth() const -> float override
	{
		DWRITE_TEXT_METRICS metrics;
		m_handle->GetMetrics(&metrics);
		return metrics.width;
	}
	auto getMinimumHeight() const -> float override
	{
		DWRITE_OVERHANG_METRICS overhangMetrics;
		m_handle->GetOverhangMetrics(&overhangMetrics);

		return m_handle->GetMaxHeight() + overhangMetrics.bottom + m_isTopTrimmed*overhangMetrics.top;
	}

	//------------------------------

private:
	bool m_isTopTrimmed = false;
public:
	auto setIsTopTrimmed(bool p_isTopTrimmed) -> void override
	{
		m_isTopTrimmed = p_isTopTrimmed;
	}
	auto getIsTopTrimmed() const -> bool override
	{
		return m_isTopTrimmed;
	}

	//------------------------------

	auto getCharacterPosition(Index p_characterIndex, bool p_isRelativeToOrigin) const -> Avo::Point<> override
	{
		Avo::Point<> result;
		DWRITE_HIT_TEST_METRICS metrics;
		m_handle->HitTestTextPosition(
			Avo::getUnitIndexFromCharacterIndex(std::u16string_view{m_wideString}, p_characterIndex), 
			false, &result.x, &result.y, &metrics
		);
		if (p_isRelativeToOrigin)
		{
			result += m_bounds.getTopLeft();
		}
		return result;
	}
	auto getCharacterSize(Index p_characterIndex) const -> Avo::Point<> override
	{
		float x;
		float y;
		DWRITE_HIT_TEST_METRICS metrics;
		m_handle->HitTestTextPosition(
			Avo::getUnitIndexFromCharacterIndex(std::u16string_view{m_wideString}, p_characterIndex), 
			false, &x, &y, &metrics
		);
		return {metrics.width, metrics.height};
	}
	auto getCharacterBounds(Index p_characterIndex, bool p_isRelativeToOrigin) const -> Avo::Rectangle<> override
	{
		Avo::Rectangle<> result;
		DWRITE_HIT_TEST_METRICS metrics;
		m_handle->HitTestTextPosition(
			Avo::getUnitIndexFromCharacterIndex(std::u16string_view{m_wideString}, p_characterIndex), 
			false, &result.left, &result.top, &metrics
		);
		if (p_isRelativeToOrigin)
		{
			result.left += m_bounds.left;
			result.top += m_bounds.top;
		}
		result.right = result.left + metrics.width;
		result.bottom = result.top + metrics.height;
		return result;
	}
	auto getNearestCharacterIndex(Avo::Point<> p_point, bool p_isRelativeToOrigin) const -> Index override
	{
		int isTrailingHit;
		int isInside;
		DWRITE_HIT_TEST_METRICS metrics;

		if (p_isRelativeToOrigin)
		{
			p_point -= m_bounds.getTopLeft();
		}
		m_handle->HitTestPoint(
			p_point.x, p_point.y, 
			&isTrailingHit, &isInside, &metrics
		);
		return Avo::getCharacterIndexFromUnitIndex(std::u16string_view{m_wideString}, metrics.textPosition) + isTrailingHit;// *isInside;
	}
	auto getNearestCharacterIndexAndPosition(Avo::Point<> p_point, bool p_isRelativeToOrigin) const -> std::pair<Index, Avo::Point<>> override
	{
		int isTrailingHit;
		int isInside;
		DWRITE_HIT_TEST_METRICS metrics;

		if (p_isRelativeToOrigin)
		{
			p_point -= m_bounds.getTopLeft();
		}
		m_handle->HitTestPoint(
			p_point.x, p_point.y, 
			&isTrailingHit, &isInside, &metrics
		);
		return {
			Avo::getCharacterIndexFromUnitIndex(std::u16string_view{m_wideString}, metrics.textPosition) + isTrailingHit,// *isInside,
			Avo::Point<>{
				metrics.left + isTrailingHit*metrics.width + p_isRelativeToOrigin*m_bounds.left, 
				metrics.top + p_isRelativeToOrigin*m_bounds.top
			}
		};
	}
	auto getNearestCharacterIndexAndBounds(Avo::Point<> p_point, bool p_isRelativeToOrigin) const 
		-> std::pair<Index, Avo::Rectangle<>> override
	{
		int isTrailingHit;
		int isInside;
		DWRITE_HIT_TEST_METRICS metrics;

		if (p_isRelativeToOrigin)
		{
			p_point -= m_bounds.getTopLeft();
		}
		m_handle->HitTestPoint(
			p_point.x, p_point.y, 
			&isTrailingHit, &isInside, &metrics
		);

		auto bounds = Avo::Rectangle<>{
			metrics.left + isTrailingHit*metrics.width + p_isRelativeToOrigin*m_bounds.left,
			metrics.top + p_isRelativeToOrigin*m_bounds.top
		};
		bounds.right = bounds.left + metrics.width;
		bounds.bottom = bounds.top + metrics.height;

		return {
			Avo::getCharacterIndexFromUnitIndex(std::u16string_view{m_wideString}, metrics.textPosition) + isTrailingHit,
			bounds
		};
	}

	//------------------------------

	static auto convertTextAlignToDirectWrite(Avo::TextAlign p_textAlign)
	{
		switch (p_textAlign)
		{
			case Avo::TextAlign::Left:
				return DWRITE_TEXT_ALIGNMENT_LEADING;
			case Avo::TextAlign::Center:
				return DWRITE_TEXT_ALIGNMENT_CENTER;
			case Avo::TextAlign::Right:
				return DWRITE_TEXT_ALIGNMENT_TRAILING;
			case Avo::TextAlign::Fill:
				return DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
		}
		return static_cast<DWRITE_TEXT_ALIGNMENT>(-1);
	}

	auto setTextAlign(Avo::TextAlign p_textAlign) -> void override
	{
		m_handle->SetTextAlignment(convertTextAlignToDirectWrite(p_textAlign));
	}
	auto getTextAlign() const -> Avo::TextAlign override
	{
		switch (m_handle->GetTextAlignment())
		{
			case DWRITE_TEXT_ALIGNMENT_LEADING:
				return Avo::TextAlign::Left;
			case DWRITE_TEXT_ALIGNMENT_CENTER:
				return Avo::TextAlign::Center;
			case DWRITE_TEXT_ALIGNMENT_TRAILING:
				return Avo::TextAlign::Right;
			case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
				return Avo::TextAlign::Fill;
		}
		return Avo::TextAlign::Unknown;
	}

	static auto convertReadingDirectionToDirectWrite(Avo::ReadingDirection p_readingDirection)
	{
		switch (p_readingDirection)
		{
			case Avo::ReadingDirection::LeftToRight:
				return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
			case Avo::ReadingDirection::RightToLeft:
				return DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
			case Avo::ReadingDirection::TopToBottom:
				return DWRITE_READING_DIRECTION_TOP_TO_BOTTOM;
			case Avo::ReadingDirection::BottomToTop:
				return DWRITE_READING_DIRECTION_BOTTOM_TO_TOP;
		}
		return static_cast<DWRITE_READING_DIRECTION>(-1);
	}

	auto setReadingDirection(Avo::ReadingDirection p_readingDirection) -> void override
	{
		m_handle->SetReadingDirection(convertReadingDirectionToDirectWrite(p_readingDirection));
	}
	auto getReadingDirection() const -> Avo::ReadingDirection override
	{
		switch (m_handle->GetReadingDirection())
		{
			case DWRITE_READING_DIRECTION_LEFT_TO_RIGHT:
				return Avo::ReadingDirection::LeftToRight;
			case DWRITE_READING_DIRECTION_RIGHT_TO_LEFT:
				return Avo::ReadingDirection::RightToLeft;
			case DWRITE_READING_DIRECTION_TOP_TO_BOTTOM:
				return Avo::ReadingDirection::TopToBottom;
			case DWRITE_READING_DIRECTION_BOTTOM_TO_TOP:
				return Avo::ReadingDirection::BottomToTop;
		}
		return Avo::ReadingDirection::Unknown;
	}

	//------------------------------

	auto setFontFamily(std::string_view const p_name, TextRange const p_range) -> void override
	{
		char16 wideName[100];
		Avo::convertUtf8ToUtf16(p_name, wideName);

		m_handle->SetFontFamilyName(reinterpret_cast<wchar_t const*>(wideName), convertTextRange(p_range));
	}

	//------------------------------

	auto setCharacterSpacing(float const p_leading, float const p_trailing, TextRange const p_range) -> void override
	{
		m_handle->SetCharacterSpacing(p_leading, p_trailing, 0.f, convertTextRange(p_range));
	}
	auto getLeadingCharacterSpacing(Index const p_characterIndex) const -> float override
	{
		float leadingSpacing;
		m_handle->GetCharacterSpacing(p_characterIndex, &leadingSpacing, 0, 0);
		return leadingSpacing;
	}
	auto getTrailingCharacterSpacing(Index const p_characterIndex) const -> float override
	{
		float trailingSpacing;
		m_handle->GetCharacterSpacing(p_characterIndex, 0, &trailingSpacing, 0);
		return trailingSpacing;
	}

	//------------------------------

	auto setLineHeight(float const p_lineHeight) -> void override
	{
		m_handle->SetLineSpacing(DWRITE_LINE_SPACING_METHOD::DWRITE_LINE_SPACING_METHOD_PROPORTIONAL, p_lineHeight, p_lineHeight*0.8f);
	}
	auto getLineHeight() const -> float override
	{
		DWRITE_LINE_SPACING_METHOD method;
		float height;
		float baseline;
		m_handle->GetLineSpacing(&method, &height, &baseline);
		return height;
	}

	//------------------------------

	auto setFontWeight(Avo::FontWeight const p_fontWeight, TextRange const p_range) -> void override
	{
		m_handle->SetFontWeight(static_cast<DWRITE_FONT_WEIGHT>(p_fontWeight), convertTextRange(p_range));
	}
	auto getFontWeight(Index const p_characterPosition) const -> Avo::FontWeight override
	{
		DWRITE_FONT_WEIGHT fontWeight;
		m_handle->GetFontWeight(p_characterPosition, &fontWeight);
		return static_cast<Avo::FontWeight>(fontWeight);
	}

	//------------------------------

	auto setFontStyle(Avo::FontStyle const p_fontStyle, TextRange const p_range) -> void override
	{
		m_handle->SetFontStyle(static_cast<DWRITE_FONT_STYLE>(p_fontStyle), convertTextRange(p_range));
	}
	auto getFontStyle(Index const p_characterPosition) const -> Avo::FontStyle override
	{
		DWRITE_FONT_STYLE directwriteFontStyle;
		m_handle->GetFontStyle(p_characterPosition, &directwriteFontStyle);
		return static_cast<Avo::FontStyle>(directwriteFontStyle);
	}

	//------------------------------

	auto setFontStretch(Avo::FontStretch const p_fontStretch, TextRange const p_range) -> void override
	{
		m_handle->SetFontStretch(static_cast<DWRITE_FONT_STRETCH>(p_fontStretch), convertTextRange(p_range));
	}
	auto getFontStretch(Index const p_characterPosition) -> Avo::FontStretch
	{
		DWRITE_FONT_STRETCH fontStretch;
		m_handle->GetFontStretch(p_characterPosition, &fontStretch);
		return static_cast<Avo::FontStretch>(fontStretch);
	}

	//------------------------------

	auto setFontSize(float const p_fontSize, TextRange const p_range) -> void override
	{
		m_handle->SetFontSize(p_fontSize, convertTextRange(p_range));
	}
	auto getFontSize(Index const p_characterPosition) const -> float override
	{
		float fontSize;
		m_handle->GetFontSize(p_characterPosition, &fontSize);
		return fontSize;
	}

	//------------------------------

private:
	std::u16string m_wideString;
	std::string m_string;
public:
	auto getString() const -> std::string_view override
	{
		return m_string;
	}

	//------------------------------

private:
	ComReference<IDWriteTextLayout1> m_handle;
public:
	auto getDWriteTextLayout() const -> IDWriteTextLayout1*
	{
		return m_handle;
	}

	//------------------------------

	DirectWriteText(
		IDWriteTextLayout1* const p_handle, std::u16string_view const p_wideString, 
	    std::string_view const p_string, Avo::Rectangle<float> const p_bounds
	) : 
		m_handle{p_handle},
		m_wideString{p_wideString},
		m_string{p_string}
	{
		m_bounds = p_bounds; // Belongs to ProtectedRectangle
		if (!m_bounds.right && !m_bounds.bottom)
		{
			m_handle->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			fitSizeToText();
		}
		else
		{
			m_handle->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
		}
	}
};

//class FontData
//{
//public:
//	void const* data;
//	uint32 dataSize;
//
//	FontData(void const* p_data, uint32 p_dataSize) :
//		data(p_data), dataSize(p_dataSize)
//	{}
//};
using FontData = std::shared_ptr<Avo::DataVector>;

class FontFileStream : public IDWriteFontFileStream
{
	IUnknownDefinition(IDWriteFontFileStream)

	//------------------------------

private:
	FontData m_fontData;
public:
	auto __stdcall ReadFileFragment(
		void const** const p_fragment, UINT64 const p_fileOffset, 
	    UINT64 const p_fragmentSize, void** const p_fragmentContext
	) -> HRESULT override
	{
		if (p_fileOffset + p_fragmentSize > m_fontData->size() || !p_fragmentSize)
		{
			*p_fragment = nullptr;
			*p_fragmentContext = nullptr;
			return E_FAIL;
		}

		*p_fragment = m_fontData->data() + p_fileOffset;
		*p_fragmentContext = nullptr;

		return S_OK;
	}
	auto __stdcall ReleaseFileFragment(void* const p_fragmentContext) -> void override {}

	auto __stdcall GetFileSize(UINT64* const p_fileSize) -> HRESULT override
	{
		*p_fileSize = m_fontData->size();
		return S_OK;
	}
	auto __stdcall GetLastWriteTime(UINT64* const p_lastWriteTime) -> HRESULT override
	{
		*p_lastWriteTime = {};
		return E_NOTIMPL;
	}

	//------------------------------

	FontFileStream(FontData const& p_fontData) :
		m_fontData{p_fontData}
	{
	}
};

class FontFileLoader : public IDWriteFontFileLoader
{
	IUnknownDefinition(IDWriteFontFileLoader)

	//------------------------------

	auto __stdcall CreateStreamFromKey(
		void const* const p_data, UINT32 const p_dataSize, 
	    IDWriteFontFileStream** const p_stream) -> HRESULT override
	{
		if (p_dataSize != sizeof(FontData) || !p_data)
		{
			*p_stream = nullptr;
			return E_INVALIDARG;
		}
		*p_stream = new FontFileStream{*static_cast<FontData const*>(p_data)};
		return S_OK;
	}
};
class FontFileEnumerator : public IDWriteFontFileEnumerator
{
	IUnknownDefinition(IDWriteFontFileEnumerator)

	//------------------------------

private:
	IDWriteFontFile* m_currentFontFile = nullptr;
public:
	auto __stdcall GetCurrentFontFile(IDWriteFontFile** const p_fontFile) -> HRESULT override
	{
		*p_fontFile = m_currentFontFile;
		if (m_currentFontFile)
		{
			return S_OK;
		}
		return E_FAIL;
	}

private:
	IDWriteFactory* m_factory;
	FontFileLoader* m_fontFileLoader;

	std::vector<FontData> const* m_fontData;
	Index m_currentFontFileIndex = -1;
public:
	auto __stdcall MoveNext(BOOL* const p_hasCurrentFile) -> HRESULT override
	{
		m_currentFontFileIndex++;

		if (m_currentFontFileIndex >= m_fontData->size())
		{
			*p_hasCurrentFile = false;
			m_currentFontFile = nullptr;
		}
		else
		{
			*p_hasCurrentFile = 1;
			m_factory->CreateCustomFontFileReference(
				static_cast<void const*>(m_fontData->data() + m_currentFontFileIndex), 
				sizeof(FontData), m_fontFileLoader, &m_currentFontFile
			);
		}
		return S_OK;
	}

public:
	FontFileEnumerator(
		IDWriteFactory* const p_factory, 
		FontFileLoader* const p_fontFileLoader, 
		std::vector<FontData> const* const p_data
	) :
		m_factory{p_factory},
		m_fontFileLoader{p_fontFileLoader},
		m_fontData{p_data}
	{}
};

class FontCollectionLoader : public IDWriteFontCollectionLoader
{
	IUnknownDefinition(IDWriteFontCollectionLoader)

	//------------------------------

	auto __stdcall CreateEnumeratorFromKey(
		IDWriteFactory* const p_factory, 
		void const* const p_data, UINT32 const p_dataSize, 
	    IDWriteFontFileEnumerator** const p_fontFileEnumerator
	) -> HRESULT
	{
		*p_fontFileEnumerator = new FontFileEnumerator{
			p_factory, m_fontFileLoader, 
			static_cast<std::vector<FontData> const*>(p_data)
		};
		return S_OK;
	}

private:
	FontFileLoader* m_fontFileLoader;
public:
	FontCollectionLoader(FontFileLoader* const p_fontFileLoader) :
		m_fontFileLoader{p_fontFileLoader}
	{
	}
};

//------------------------------

class Direct2dGeometry : public Avo::Geometry
{
private:
	ComReference<ID2D1GeometryRealization> m_strokedRealization;

public:
	auto setStrokedRealization(ComReference<ID2D1GeometryRealization> const& p_realization) -> void
	{
		m_strokedRealization = p_realization;
	}
	auto getStrokedRealization() const -> ID2D1GeometryRealization*
	{
		return m_strokedRealization;
	}

private:
	ComReference<ID2D1GeometryRealization> m_filledRealization;
public:
	auto setFilledRealization(ComReference<ID2D1GeometryRealization> const& p_realization) -> void
	{
		m_filledRealization = p_realization;
	}
	auto getFilledRealization() const -> ID2D1GeometryRealization*
	{
		return m_filledRealization;
	}

private:
	ComReference<ID2D1Geometry> m_geometry;
public:
	auto getGeometry() const -> ID2D1Geometry*
	{
		return m_geometry;
	}

	Direct2dGeometry(ComReference<ID2D1Geometry> const& p_geometry) :
		m_geometry{p_geometry}
	{
	}
};

//------------------------------

struct Direct2dLinearGradient : public Avo::LinearGradient
{
	auto setOffset(Avo::Point<> const p_offset) -> void override
	{
		m_brush->SetTransform(D2D1::Matrix3x2F::Translation(p_offset.x, p_offset.y));
	}
	auto setOffsetX(Avo::Dip const p_x) -> void override
	{
		m_brush->SetTransform(D2D1::Matrix3x2F::Translation(p_x, getOffsetY()));
	}
	auto setOffsetY(Avo::Dip const p_y) -> void override
	{
		m_brush->SetTransform(D2D1::Matrix3x2F::Translation(getOffsetX(), p_y));
	}
	auto getOffset() const -> Avo::Point<> override
	{
		D2D1::Matrix3x2F matrix;
		m_brush->GetTransform(&matrix);
		return Avo::Point<>(matrix._31, matrix._32);
	}
	auto getOffsetX() const -> float override
	{
		D2D1::Matrix3x2F matrix;
		m_brush->GetTransform(&matrix);
		return matrix._31;
	}
	auto getOffsetY() const -> float override
	{
		D2D1::Matrix3x2F matrix;
		m_brush->GetTransform(&matrix);
		return matrix._32;
	}

	auto setStartPosition(Avo::Point<> const p_position) -> void override
	{
		m_brush->SetStartPoint({p_position.x, p_position.y});
	}
	auto getStartPosition() const -> Avo::Point<> override
	{
		return {m_brush->GetStartPoint().x, m_brush->GetStartPoint().y};
	}
	auto getStartPositionX() const -> Avo::Dip override
	{
		return m_brush->GetStartPoint().x;
	}
	auto getStartPositionY() const -> Avo::Dip override
	{
		return m_brush->GetStartPoint().y;
	}

	auto setEndPosition(Avo::Point<> const p_position) -> void override
	{
		m_brush->SetEndPoint({p_position.x, p_position.y});
	}
	auto getEndPosition() const -> Avo::Point<> override
	{
		return {m_brush->GetEndPoint().x, m_brush->GetEndPoint().y};
	}
	auto getEndPositionX() const -> float override
	{
		return m_brush->GetEndPoint().x;
	}
	auto getEndPositionY() const -> float override
	{
		return m_brush->GetEndPoint().y;
	}

private:
	ComReference<ID2D1LinearGradientBrush> m_brush;

public:
	auto getD2dBrush() const -> ID2D1LinearGradientBrush*
	{
		return m_brush;
	}

	Direct2dLinearGradient(ComReference<ID2D1LinearGradientBrush> const& p_brush) :
		m_brush{p_brush}
	{
	}
};

//------------------------------

struct Direct2dRadialGradient : public Avo::RadialGradient
{
	auto setOffset(Avo::Point<> const p_offset) -> void override
	{
		m_brush->SetTransform(D2D1::Matrix3x2F::Translation(p_offset.x, p_offset.y));
	}
	auto setOffsetX(Avo::Dip const p_x) -> void override
	{
		m_brush->SetTransform(D2D1::Matrix3x2F::Translation(p_x, getOffsetY()));
	}
	auto setOffsetY(Avo::Dip const p_y) -> void override
	{
		m_brush->SetTransform(D2D1::Matrix3x2F::Translation(getOffsetX(), p_y));
	}
	auto getOffset() const -> Avo::Point<> override
	{
		D2D1::Matrix3x2F matrix;
		m_brush->GetTransform(&matrix);
		return {matrix._31, matrix._32};
	}
	auto getOffsetX() const -> float override
	{
		D2D1::Matrix3x2F matrix;
		m_brush->GetTransform(&matrix);
		return matrix._31;
	}
	auto getOffsetY() const -> float override
	{
		D2D1::Matrix3x2F matrix;
		m_brush->GetTransform(&matrix);
		return matrix._32;
	}

	auto setStartPosition(Avo::Point<> const p_offset) -> void override
	{
		m_brush->SetCenter({p_offset.x, p_offset.y});
	}
	auto getStartPosition() const -> Avo::Point<> override
	{
		return {m_brush->GetCenter().x, m_brush->GetCenter().y};
	}
	auto getStartPositionX() const -> Avo::Dip override
	{
		return m_brush->GetCenter().x;
	}
	auto getStartPositionY() const -> Avo::Dip override
	{
		return m_brush->GetCenter().y;
	}

	auto setRadius(Avo::Size<> const p_radius) -> void override
	{
		m_brush->SetRadiusX(p_radius.x);
		m_brush->SetRadiusY(p_radius.y);
	}
	auto getRadius() const -> Avo::Size<> override
	{
		return {m_brush->GetRadiusX(), m_brush->GetRadiusY()};
	}
	auto getRadiusX() const -> float override
	{
		return m_brush->GetRadiusX();
	}
	auto getRadiusY() const -> float override
	{
		return m_brush->GetRadiusY();
	}

private:
	ComReference<ID2D1RadialGradientBrush> m_brush;
public:
	auto getD2dBrush() const -> ID2D1RadialGradientBrush*
	{
		return m_brush;
	}

	Direct2dRadialGradient(ComReference<ID2D1RadialGradientBrush> const& p_brush) :
		m_brush{p_brush}
	{}
};

//------------------------------

struct Direct2DDrawingState : public Avo::DrawingState
{
private:
	ComReference<ID2D1DrawingStateBlock1> m_drawingState;

public:
	auto getD2dStateBlock() -> ID2D1DrawingStateBlock1*
	{
		return m_drawingState;
	}

	Direct2DDrawingState(ComReference<ID2D1DrawingStateBlock1> const& p_drawingState) :
		m_drawingState{p_drawingState}
	{}
};

//------------------------------

class Direct2dDrawingContext : public Avo::DrawingContext
{
public:
	static ComReference<IWICImagingFactory2> s_imagingFactory;
	static ComReference<ID2D1Factory2> s_direct2DFactory;
	static ComReference<IDWriteFactory1> s_directWriteFactory;

	static ComReference<FontCollectionLoader> s_fontCollectionLoader;
	static ComReference<FontFileLoader> s_fontFileLoader;

	static auto createStaticResources() -> void
	{
		if (!s_imagingFactory)
		{
			CoInitialize(0);
			CoCreateInstance(CLSID_WICImagingFactory2, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_imagingFactory));
		}

		//------------------------------

		if (!s_direct2DFactory)
		{
#ifdef _DEBUG
			D2D1_FACTORY_OPTIONS options;
			options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
			D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options, &s_direct2DFactory);
#else
			D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &s_direct2DFactory);
#endif
		}

		if (!s_directWriteFactory)
		{
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(s_directWriteFactory), 
				reinterpret_cast<IUnknown**>(&s_directWriteFactory)
			);

			s_fontFileLoader = makeComReference<FontFileLoader>();
			s_directWriteFactory->RegisterFontFileLoader(s_fontFileLoader);

			s_fontCollectionLoader = makeComReference<FontCollectionLoader>(s_fontFileLoader);
			s_directWriteFactory->RegisterFontCollectionLoader(s_fontCollectionLoader);
		}
	}
	static auto destroyStaticResources() -> void
	{
		if (!WindowsWindow::s_numberOfWindows)
		{
			if (s_directWriteFactory)
			{
				s_directWriteFactory->UnregisterFontCollectionLoader(s_fontCollectionLoader);
				s_directWriteFactory->UnregisterFontFileLoader(s_fontFileLoader);
			}
			s_fontFileLoader = nullptr;
			s_fontCollectionLoader = nullptr;
			s_directWriteFactory = nullptr;
			s_direct2DFactory = nullptr;
			s_imagingFactory = nullptr;
		}
	}

	struct Direct2dLock
	{
		ComReference<ID2D1Multithread> multithreader;
		Direct2dLock()
		{
			s_direct2DFactory->QueryInterface(IID_PPV_ARGS(&multithreader));
			multithreader->Enter();
		}
		~Direct2dLock()
		{
			multithreader->Leave();
		}
		
		Direct2dLock(Direct2dLock const&) = delete;
		Direct2dLock(Direct2dLock&&) = delete;
		auto operator=(Direct2dLock const&) -> auto& = delete;
		auto operator=(Direct2dLock&&) -> auto& = delete;
	};

private:
	Avo::Window* m_window = nullptr;

	ComReference<ID2D1DeviceContext1> m_context;
	ComReference<IDXGISwapChain1> m_swapChain;

	ComReference<ID2D1SolidColorBrush> m_solidColorBrush;
	ComReference<ID2D1Brush> m_currentBrush;
	float m_brushOpacity = 1.f;

	//------------------------------

	auto createCornerRectanglePathGeometry(
		Avo::Rectangle<> const p_rectangle, 
		Avo::RectangleCorners const& p_corners, 
		bool const p_isFilled
	) -> ComReference<ID2D1PathGeometry1>
	{
		ComReference<ID2D1PathGeometry1> geometry;
		s_direct2DFactory->CreatePathGeometry(&geometry);

		ComReference<ID2D1GeometrySink> sink;
		geometry->Open(&sink);

		sink->SetFillMode(D2D1_FILL_MODE_WINDING);
		sink->BeginFigure(
			{p_rectangle.left, p_rectangle.top + p_corners.topLeftSizeY}, 
			p_isFilled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW
		);

		if (p_corners.topLeftSizeX && p_corners.topLeftSizeY)
		{
			if (p_corners.topLeftType == Avo::RectangleCornerType::Round)
			{
				sink->AddArc(
					D2D1::ArcSegment(
						{p_rectangle.left + p_corners.topLeftSizeX, p_rectangle.top}, 
						{p_corners.topLeftSizeX, p_corners.topLeftSizeY}, 
						0.f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL
					)
				);
			}
			else
			{
				sink->AddLine({p_rectangle.left + p_corners.topLeftSizeX, p_rectangle.top});
			}
		}
		sink->AddLine({p_rectangle.right - p_corners.topRightSizeX, p_rectangle.top});
		if (p_corners.topRightSizeX && p_corners.topRightSizeY)
		{
			if (p_corners.topRightType == Avo::RectangleCornerType::Round)
			{
				sink->AddArc(
					D2D1::ArcSegment(
						{p_rectangle.right, p_rectangle.top + p_corners.topRightSizeY}, 
						{p_corners.topRightSizeX, p_corners.topRightSizeY}, 
						0.f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL
					)
				);
			}
			else
			{
				sink->AddLine({p_rectangle.right, p_rectangle.top + p_corners.topRightSizeY});
			}
		}
		sink->AddLine({p_rectangle.right, p_rectangle.bottom - p_corners.bottomRightSizeY});
		if (p_corners.bottomRightSizeX && p_corners.bottomRightSizeY)
		{
			if (p_corners.bottomRightType == Avo::RectangleCornerType::Round)
			{
				sink->AddArc(
					D2D1::ArcSegment(
						{p_rectangle.right - p_corners.bottomRightSizeX, p_rectangle.bottom}, 
						{p_corners.bottomRightSizeX, p_corners.bottomRightSizeY}, 
						0.f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL
					)
				);
			}
			else
			{
				sink->AddLine({p_rectangle.right - p_corners.bottomRightSizeX, p_rectangle.bottom});
			}
		}
		sink->AddLine({p_rectangle.left + p_corners.bottomLeftSizeX, p_rectangle.bottom});
		if (p_corners.bottomLeftSizeX && p_corners.bottomLeftSizeY)
		{
			if (p_corners.bottomLeftType == Avo::RectangleCornerType::Round)
			{
				sink->AddArc(
					D2D1::ArcSegment(
						{p_rectangle.left, p_rectangle.bottom - p_corners.bottomLeftSizeY}, 
						{p_corners.bottomLeftSizeX, p_corners.bottomLeftSizeY}, 
						0.f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL
					)
				);
			}
			else
			{
				sink->AddLine({p_rectangle.left, p_rectangle.bottom - p_corners.bottomLeftSizeY});
			}
		}
		sink->AddLine({p_rectangle.left, p_rectangle.top + p_corners.topLeftSizeY});

		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		sink->Close();

		return geometry;
	}

	D2D1_STROKE_STYLE_PROPERTIES1 m_strokeStyleProperties;
	ComReference<ID2D1StrokeStyle1> m_strokeStyle;
	auto updateStrokeStyle() -> void
	{
		m_strokeStyle = nullptr; // Releases, important
		s_direct2DFactory->CreateStrokeStyle(m_strokeStyleProperties, 0, 0, &m_strokeStyle);
	}

	//------------------------------

	auto realizeStrokedGeometry(Direct2dGeometry* const p_geometry, float const p_strokeWidth) -> void
	{
		if (!p_geometry->getStrokedRealization())
		{
			D2D1_MATRIX_3X2_F transform;
			m_context->GetTransform(&transform);

			auto dpiX = float{USER_DEFAULT_SCREEN_DPI};
			auto dpiY = float{USER_DEFAULT_SCREEN_DPI};
			m_context->GetDpi(&dpiX, &dpiY);

			auto geometryRealization = ComReference<ID2D1GeometryRealization>{};
			m_context->CreateStrokedGeometryRealization(
				p_geometry->getGeometry(), 
				D2D1::ComputeFlatteningTolerance(transform, dpiX, dpiY), 
				p_strokeWidth, m_strokeStyle, &geometryRealization
			);

			p_geometry->setStrokedRealization(geometryRealization);
		}
	}
	auto realizeFilledGeometry(Direct2dGeometry* const p_geometry) -> void
	{
		if (!p_geometry->getFilledRealization())
		{
			D2D1_MATRIX_3X2_F transform;
			m_context->GetTransform(&transform);

			auto dpiX = float{USER_DEFAULT_SCREEN_DPI};
			auto dpiY = float{USER_DEFAULT_SCREEN_DPI};
			m_context->GetDpi(&dpiX, &dpiY);

			auto geometryRealization = ComReference<ID2D1GeometryRealization>{};
			m_context->CreateFilledGeometryRealization(
				p_geometry->getGeometry(), 
				D2D1::ComputeFlatteningTolerance(transform, dpiX, dpiY), 
				&geometryRealization
			);

			p_geometry->setFilledRealization(geometryRealization);
		}
	}

	std::recursive_mutex m_targetMutex;
public:
	auto beginDrawing() -> void override
	{
		m_targetMutex.lock();
		m_context->BeginDraw();
	}
	auto finishDrawing(Range<Avo::Rectangle<>*> const p_updatedRectangles) -> void override
	{
		m_context->EndDraw();
		m_targetMutex.unlock();

		if (p_updatedRectangles.size())
		{
			auto presentParameters = DXGI_PRESENT_PARAMETERS{};
			presentParameters.DirtyRectsCount = p_updatedRectangles.size();

			//std::vector<RECT> updatedRects(p_updatedRectangles.size());
			auto updatedRects = std::array<RECT, 512>(); // This is more efficient than dynamic allocation... But it does feel dangerous to have an upper limit like this.

			auto const dpiFactor = getDpi()/USER_DEFAULT_SCREEN_DPI;

			auto const size = m_context->GetPixelSize();

			// If you're getting an exception below, you have three options; 
			// 1. don't invalidate so damn many rectangles
			// 2. increase the size of the static array above
			// 3. make the array above dynamic (see the commented line above there), also don't forget to free it.
			for (auto const a : Indices{p_updatedRectangles})
			{
				updatedRects[a].left = std::max(0.f, floor(p_updatedRectangles[a].left*dpiFactor));
				updatedRects[a].top = std::max(0.f, floor(p_updatedRectangles[a].top*dpiFactor));
				updatedRects[a].right = std::min(size.width, static_cast<UINT32>(ceil(p_updatedRectangles[a].right*dpiFactor)));
				updatedRects[a].bottom = std::min(size.height, static_cast<UINT32>(ceil(p_updatedRectangles[a].bottom*dpiFactor)));
			}

			presentParameters.pDirtyRects = updatedRects.data();
			presentParameters.pScrollOffset = {};
			presentParameters.pScrollRect = {};

			auto const lock = Direct2dLock{};
			m_swapChain->Present1(1, m_isVsyncEnabled ? 0 : (DXGI_PRESENT_DO_NOT_WAIT | DXGI_PRESENT_RESTART), &presentParameters);
		}
		//else
		//{
		//	// Just triggers a buffer swap.

		//	RECT rect = {0, 0, 1, 1};
		//	DXGI_PRESENT_PARAMETERS presentParameters;
		//	presentParameters.DirtyRectsCount = 1;
		//	presentParameters.pDirtyRects = &rect;
		//	presentParameters.pScrollOffset = 0;
		//	presentParameters.pScrollRect = 0;

		//	m_swapChain->Present1(1, m_isVsyncEnabled ? 0 : (DXGI_PRESENT_DO_NOT_WAIT | DXGI_PRESENT_RESTART), &presentParameters);
		//}

	}

	auto createDrawingState() -> Avo::DrawingState override
	{
		auto drawingState = ComReference<ID2D1DrawingStateBlock1>{};
		s_direct2DFactory->CreateDrawingStateBlock(&drawingState);
		return createDrawingStateFromImplementation(std::make_shared<Direct2DDrawingState>(drawingState));
	}
	auto saveDrawingState(Avo::DrawingState const& p_drawingState) -> void override
	{
		m_context->SaveDrawingState(static_cast<Direct2DDrawingState*>(p_drawingState.getImplementation())->getD2dStateBlock());
	}
	auto restoreDrawingState(Avo::DrawingState const& p_drawingState) -> void override
	{
		m_context->RestoreDrawingState(static_cast<Direct2DDrawingState*>(p_drawingState.getImplementation())->getD2dStateBlock());
	}

	//------------------------------

	auto setIsFullscreen(bool p_isFullscreen) -> void override
	{
		m_window->setIsFullscreen(p_isFullscreen);
	}
	auto switchFullscreen() -> void override
	{
		m_window->switchFullscreen();
	}
	auto getIsFullscreen() -> bool override
	{
		return m_window->getIsFullscreen();
	}

	//------------------------------

private:
	bool m_isVsyncEnabled = true;
public:
	auto enableVsync() -> void override
	{
		m_isVsyncEnabled = true;
	}
	auto disableVsync() -> void override 
	{
		m_isVsyncEnabled = false;
	}
	auto getIsVsyncEnabled() -> bool override 
	{
		return m_isVsyncEnabled;
	}

	//------------------------------

	auto setBackgroundColor(Avo::Color const p_color) -> void override
	{
		if (m_swapChain)
		{
			auto const dxgiColor = DXGI_RGBA{
				p_color.red,
				p_color.green,
				p_color.blue,
				p_color.alpha
			};

			auto const lock = Direct2dLock{};
			m_swapChain->SetBackgroundColor(&dxgiColor);
		}
	}
	auto getBackgroundColor() -> Avo::Color override
	{
		if (m_swapChain)
		{
			auto const lock = Direct2dLock{};

			auto dxgiColor = DXGI_RGBA{};
			m_swapChain->GetBackgroundColor(&dxgiColor);

			return {dxgiColor.r, dxgiColor.g, dxgiColor.b, dxgiColor.a};
		}
		return 0.5f;
	}

	//------------------------------

	auto setDpi(float const p_dpi) -> void override
	{
		m_context->SetDpi(p_dpi, p_dpi);
	}
	auto getDpi() -> float override
	{
		auto DPIX = float{};
		auto DPIY = float{};
		m_context->GetDpi(&DPIX, &DPIY);
		return DPIX;
	}

	//------------------------------

private:
	auto getD2dTransform() -> D2D1_MATRIX_3X2_F
	{
		D2D1_MATRIX_3X2_F transform;
		m_context->GetTransform(&transform);
		return transform;
	}

	template<typename Lambda>
	auto changeD2dTransform(Lambda p_change) -> void
	{
		D2D1_MATRIX_3X2_F transform;
		m_context->GetTransform(&transform);
		p_change(transform);
		m_context->SetTransform(transform);
	}
	
	//------------------------------

public:
	auto moveOrigin(Avo::Vector2d<> const p_offset) -> void override
	{
		changeD2dTransform([&](D2D1_MATRIX_3X2_F& transform){
			transform.dx += p_offset.x;
			transform.dy += p_offset.y;
		});
	}
	auto setOrigin(Avo::Point<> const p_origin) -> void override
	{
		changeD2dTransform([&](D2D1_MATRIX_3X2_F& transform){
			transform.dx = p_origin.x;
			transform.dy = p_origin.y;
		});
	}
	auto getOrigin() -> Avo::Point<> override
	{
		auto const transform = getD2dTransform();
		return {transform.dx, transform.dy};
	}

	//------------------------------

private:
	Avo::Vector2d<Avo::Factor> m_scale = 1.f;
public:
	auto scale(Avo::Vector2d<Avo::Factor> const p_scale) -> void override
	{
		changeD2dTransform([&](D2D1_MATRIX_3X2_F& transform){
			transform.m11 *= p_scale.x;
			transform.m22 *= p_scale.y;
			transform.m21 *= p_scale.x;
			transform.m12 *= p_scale.y;
			m_scale.x *= p_scale.x;
			m_scale.y *= p_scale.y;
		});
	}
	auto scale(Avo::Vector2d<Avo::Factor> const p_scale, Avo::Point<> const p_origin) -> void override
	{
		changeD2dTransform([&](D2D1_MATRIX_3X2_F& transform){
			transform.m11 *= p_scale.x;
			transform.m22 *= p_scale.y;
			transform.m21 *= p_scale.x;
			transform.m12 *= p_scale.y;
			m_scale.x *= p_scale.x;
			m_scale.y *= p_scale.y;
			transform.dx += (p_origin.x - transform.dx)*(1.f - p_scale.x);
			transform.dy += (p_origin.y - transform.dy)*(1.f - p_scale.y);
		});
	}
	auto setScale(Avo::Vector2d<Avo::Factor> const p_scale) -> void override
	{
		scale(p_scale/m_scale);	
	}
	auto setScale(Avo::Vector2d<Avo::Factor> const p_scale, Avo::Point<> const p_origin) -> void override
	{
		scale(p_scale/m_scale, p_origin);
	}
	auto getScale() -> Avo::Vector2d<Avo::Factor> override
	{
		return m_scale;
	}
	auto getScaleX() -> Avo::Factor override
	{
		return m_scale.x;
	}
	auto getScaleY() -> Avo::Factor override
	{
		return m_scale.y;
	}

	//------------------------------

	auto rotate(Avo::Arithmetic<float, Avo::Radians> const p_radians) -> void override
	{
		rotate(Avo::Degrees{p_radians});
	}
	auto rotate(Avo::Arithmetic<float, Avo::Degrees> const p_degrees) -> void override
	{
		m_context->SetTransform(getD2dTransform()*D2D1::Matrix3x2F::Rotation(p_degrees.value));
	}
	auto rotate(Avo::Arithmetic<float, Avo::Radians> const p_radians, Avo::Point<> const p_origin) -> void override
	{
		rotate(Avo::Degrees{p_radians}, p_origin);
	}
	auto rotate(Avo::Arithmetic<float, Avo::Degrees> const p_degrees, Avo::Point<> const p_origin) -> void override
	{
		m_context->SetTransform(
			getD2dTransform()*D2D1::Matrix3x2F::Rotation(p_degrees.value, {p_origin.x, p_origin.y})
		);
	}

	//------------------------------

	auto resetTransformations() -> void override
	{
		m_context->SetTransform(D2D1::Matrix3x2F::Identity());
		m_scale.x = 1.f;
		m_scale.y = 1.f;
	}

	//------------------------------

private:
	ComReference<ID2D1Bitmap1> m_targetWindowBitmap;
	
	auto createTargetWindowBitmapFromDxgiBackBuffer(IDXGISurface* const p_backBuffer, float const p_dpi) -> void
	{
		m_context->CreateBitmapFromDxgiSurface(
			p_backBuffer,
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
				p_dpi, p_dpi
			),
			&m_targetWindowBitmap
		);
	}

public:
	auto setSize(Avo::Size<> const p_size) -> void override
	{
		if (auto const oldSize = m_context->GetSize();
		    oldSize.width == p_size.x && oldSize.height == p_size.y)
		{
			return;
		}

		auto const targetLock = std::scoped_lock{m_targetMutex};

		auto const wasOldTargetWindow = [&]() -> bool {
			ComReference<ID2D1Image> oldTarget;
			m_context->GetTarget(&oldTarget); // Adds a reference which will be removed by ~ComReference()
			return oldTarget == m_targetWindowBitmap;
		}();
		if (wasOldTargetWindow)
		{
			// Need to temporarily remove the window bitmap as the target for drawing, because otherwise it 
			// won't be deleted when DXGISwapChain1::ResizeBuffers is called (which is required).
			m_context->SetTarget(nullptr);
		}
		m_targetWindowBitmap = nullptr; // Releases our reference

		auto const dpi = getDpi();

		auto newDxgiBackBuffer = ComReference<IDXGISurface>{};
		{
			// We'll be doing DXGI calls which aren't automatically protected by Direct2D. 
			auto const lock = Direct2dLock{}; 

			// Resize buffers, creating new ones
			auto const newSize = p_size*(dpi/USER_DEFAULT_SCREEN_DPI);
			m_swapChain->ResizeBuffers(0, newSize.x, newSize.y, DXGI_FORMAT_UNKNOWN, 0);

			// Get the new back buffer
			m_swapChain->GetBuffer(0, IID_PPV_ARGS(&newDxgiBackBuffer));
		}

		createTargetWindowBitmapFromDxgiBackBuffer(newDxgiBackBuffer, dpi);

		if (wasOldTargetWindow)
		{
			m_context->SetTarget(m_targetWindowBitmap);
		}
	}
	auto getSize() -> Avo::Size<> override
	{
		auto const size = m_context->GetSize();
		return {size.width, size.height};
	}

	//------------------------------

	auto clear(Avo::Color const p_color) -> void override
	{
		m_context->Clear({p_color.red, p_color.green, p_color.blue, p_color.alpha});
	}
	auto clear() -> void override
	{
		m_context->Clear({});
	}

	//------------------------------

	auto fillRectangle(Avo::Rectangle<> const p_rectangle) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->FillRectangle(convertRectangleToRectf(p_rectangle), m_currentBrush);
	}
	auto fillRectangle(Avo::Rectangle<> const p_rectangle, Avo::RectangleCorners const& p_corners) -> void override
	{
		auto const pathGeometry = createCornerRectanglePathGeometry(p_rectangle, p_corners, true);

		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->FillGeometry(pathGeometry, m_currentBrush);
	}
	auto fillRoundedRectangle(Avo::Rectangle<> const p_rectangle, Avo::Size<> const p_radius) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->FillRoundedRectangle(
			D2D1::RoundedRect(convertRectangleToRectf(p_rectangle), p_radius.width, p_radius.height), 
			m_currentBrush
		);
	}

	//------------------------------

	auto strokeRectangle(Avo::Rectangle<> const p_rectangle, float const p_strokeWidth) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawRectangle(
			convertRectangleToRectf(p_rectangle), 
			m_currentBrush, p_strokeWidth, m_strokeStyle
		);
	}
	auto strokeRectangle(
		Avo::Rectangle<> const p_rectangle, 
		Avo::RectangleCorners const& p_corners, 
		float const p_strokeWidth
	) -> void override
	{
		auto const pathGeometry = createCornerRectanglePathGeometry(p_rectangle, p_corners, false);

		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawGeometry(pathGeometry, m_currentBrush, p_strokeWidth, m_strokeStyle);
	}
	auto strokeRoundedRectangle(Avo::Rectangle<> const p_rectangle, Avo::Size<> const p_radius, Avo::Dip const p_strokeWidth) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawRoundedRectangle(
			D2D1::RoundedRect(convertRectangleToRectf(p_rectangle), p_radius.width, p_radius.height), 
			m_currentBrush, p_strokeWidth, m_strokeStyle
		);
	}

	//------------------------------

	auto fillCircle(Avo::Point<> p_center, Avo::Dip p_radius) -> void override
	{
		fillEllipse(p_center, p_radius);
	}
	auto fillEllipse(Avo::Point<> p_center, Avo::Size<> p_radius) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->FillEllipse(
			D2D1::Ellipse({p_center.x, p_center.y}, p_radius.width, p_radius.height), 
			m_currentBrush
		);
	}

	auto strokeCircle(Avo::Point<> p_center, Avo::Dip p_radius, Avo::Dip p_strokeWidth) -> void override
	{
		strokeEllipse(p_center, p_radius, p_strokeWidth);
	}
	auto strokeEllipse(Avo::Point<> p_center, Avo::Size<> p_radius, Avo::Dip p_strokeWidth) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawEllipse(
			D2D1::Ellipse({p_center.x, p_center.y}, p_radius.x, p_radius.y), 
			m_currentBrush, p_strokeWidth, m_strokeStyle
		);
	}

	//------------------------------

	auto drawLine(Avo::Point<> p_0, Avo::Point<> p_1, Avo::Dip p_thickness) -> void override
	{
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawLine(
			{p_0.x, p_0.y},
			{p_1.x, p_1.y},
			m_currentBrush,
			p_thickness, m_strokeStyle
		);
	}

	//------------------------------

private:
	static auto createPathGeometry(Range<Avo::Point<> const*> const p_vertices, bool const p_isStroked, bool const p_isClosed)
		-> ComReference<ID2D1PathGeometry1>
	{
		auto path = ComReference<ID2D1PathGeometry1>{};
		s_direct2DFactory->CreatePathGeometry(&path);

		auto sink = ComReference<ID2D1GeometrySink>{};
		path->Open(&sink);

		sink->BeginFigure({p_vertices[0].x, p_vertices[0].y}, p_isStroked ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED);
		for (auto const a : Indices{1, p_vertices})
		{
			sink->AddLine({p_vertices[a].x, p_vertices[a].y});
		}
		sink->EndFigure(p_isClosed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);

		sink->Close();
		return path;
	}
	static auto getD2dGeometryFromGeometry(Avo::Geometry const& p_geometry) -> Direct2dGeometry*
	{
		return static_cast<Direct2dGeometry*>(p_geometry.getImplementation());
	}

public:
	auto strokeShape(Range<Avo::Point<>*> const p_vertices, float const p_lineThickness, bool const p_isClosed) -> void override
	{
		if (!p_vertices.size())
		{
			return;
		}

		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawGeometry(createPathGeometry(p_vertices, true, p_isClosed), m_currentBrush, p_lineThickness, m_strokeStyle);
	}

	auto fillShape(Range<Avo::Point<>*> const p_vertices) -> void override
	{
		if (!p_vertices.size())
		{
			return;
		}

		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->FillGeometry(createPathGeometry(p_vertices, false, true), m_currentBrush);
	}

	//------------------------------

	auto strokeGeometry(Avo::Geometry const& p_geometry, float const p_strokeWidth) -> void override
	{
		auto* const implementation = getD2dGeometryFromGeometry(p_geometry);
		if (!implementation->getStrokedRealization())
		{
			realizeStrokedGeometry(implementation, p_strokeWidth);
		}
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawGeometryRealization(implementation->getStrokedRealization(), m_currentBrush);
	}
	auto fillGeometry(Avo::Geometry const& p_geometry) -> void override
	{
		auto* const implementation = getD2dGeometryFromGeometry(p_geometry);
		if (!implementation->getFilledRealization())
		{
			realizeFilledGeometry(implementation);
		}
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawGeometryRealization(implementation->getFilledRealization(), m_currentBrush);
	}

	//------------------------------

	auto createRoundedRectangleGeometry(Avo::Rectangle<> const p_rectangle, Avo::Dip const p_radius, bool const p_isStroked) -> Avo::Geometry override
	{
		auto geometry = ComReference<ID2D1RoundedRectangleGeometry>{};
		s_direct2DFactory->CreateRoundedRectangleGeometry(
			D2D1::RoundedRect(convertRectangleToRectf(p_rectangle), p_radius, p_radius), &geometry
		);

		return createGeometryFromImplementation(std::make_shared<Direct2dGeometry>(geometry.as<ID2D1Geometry>()));
	}

	auto createCornerRectangleGeometry(Avo::Rectangle<> p_rectangle, Avo::RectangleCorners const& p_corners, bool p_isStroked) -> Avo::Geometry override
	{
		return createGeometryFromImplementation(std::make_shared<Direct2dGeometry>(
			createCornerRectanglePathGeometry(p_rectangle, p_corners, true).as<ID2D1Geometry>()
		));
	}

	//------------------------------

	auto createPolygonGeometry(Range<Avo::Point<> const*> p_vertices, bool p_isStroked, bool p_isClosed) -> Avo::Geometry override
	{
		if (!p_vertices.size())
		{
			return {};
		}

		return createGeometryFromImplementation(std::make_shared<Direct2dGeometry>(
			createPathGeometry(p_vertices, p_isStroked, p_isClosed).as<ID2D1Geometry>()
		));
	}

	//------------------------------

	auto setLineCap(Avo::LineCap p_lineCap) -> void override
	{
		switch (p_lineCap)
		{
			case Avo::LineCap::Flat:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_FLAT;
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_FLAT;
				break;
			case Avo::LineCap::Round:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_ROUND;
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_ROUND;
				break;
			case Avo::LineCap::Square:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_SQUARE;
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_SQUARE;
				break;
			case Avo::LineCap::Triangle:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_TRIANGLE;
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_TRIANGLE;
		}
		updateStrokeStyle();
	}
	auto setStartLineCap(Avo::LineCap p_lineCap) -> void override
	{
		switch (p_lineCap)
		{
			case Avo::LineCap::Flat:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_FLAT;
				break;
			case Avo::LineCap::Round:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_ROUND;
				break;
			case Avo::LineCap::Square:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_SQUARE;
				break;
			case Avo::LineCap::Triangle:
				m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_TRIANGLE;
		}
		updateStrokeStyle();
	}
	auto setEndLineCap(Avo::LineCap p_lineCap) -> void override
	{
		switch (p_lineCap)
		{
			case Avo::LineCap::Flat:
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_FLAT;
				break;
			case Avo::LineCap::Round:
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_ROUND;
				break;
			case Avo::LineCap::Square:
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_SQUARE;
				break;
			case Avo::LineCap::Triangle:
				m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_TRIANGLE;
		}
		updateStrokeStyle();
	}
	auto getStartLineCap() -> Avo::LineCap override
	{
		switch (m_strokeStyleProperties.startCap)
		{
			case D2D1_CAP_STYLE_FLAT:
				return Avo::LineCap::Flat;
			case D2D1_CAP_STYLE_ROUND:
				return Avo::LineCap::Round;
			case D2D1_CAP_STYLE_SQUARE:
				return Avo::LineCap::Square;
			case D2D1_CAP_STYLE_TRIANGLE:
				return Avo::LineCap::Triangle;
		}
		return Avo::LineCap::Triangle;
	}
	auto getEndLineCap() -> Avo::LineCap override
	{
		switch (m_strokeStyleProperties.endCap)
		{
			case D2D1_CAP_STYLE_FLAT:
				return Avo::LineCap::Flat;
			case D2D1_CAP_STYLE_ROUND:
				return Avo::LineCap::Round;
			case D2D1_CAP_STYLE_SQUARE:
				return Avo::LineCap::Square;
			case D2D1_CAP_STYLE_TRIANGLE:
				return Avo::LineCap::Triangle;
		}
		return Avo::LineCap::Triangle;
	}

	//------------------------------

	auto setLineJoin(Avo::LineJoin p_lineJoin) -> void override
	{
		switch (p_lineJoin)
		{
			case Avo::LineJoin::Bevel:
				m_strokeStyleProperties.lineJoin = D2D1_LINE_JOIN_BEVEL;
				break;
			case Avo::LineJoin::Miter:
				m_strokeStyleProperties.lineJoin = D2D1_LINE_JOIN_MITER;
				break;
			case Avo::LineJoin::Round:
				m_strokeStyleProperties.lineJoin = D2D1_LINE_JOIN_ROUND;
				break;
		}
		updateStrokeStyle();
	}
	auto getLineJoin() -> Avo::LineJoin override
	{
		switch (m_strokeStyleProperties.lineJoin)
		{
			case D2D1_LINE_JOIN_BEVEL:
				return Avo::LineJoin::Bevel;
			case D2D1_LINE_JOIN_MITER:
				return Avo::LineJoin::Miter;
			case D2D1_LINE_JOIN_ROUND:
				return Avo::LineJoin::Round;
		}
		return Avo::LineJoin::Miter;
	}

	auto setLineJoinMiterLimit(float p_miterLimit) -> void override
	{
		m_strokeStyleProperties.miterLimit = p_miterLimit;
		updateStrokeStyle();
	}
	auto getLineJoinMiterLimit() -> float override
	{
		return m_strokeStyleProperties.miterLimit;
	}

	//------------------------------

	auto setLineDashStyle(Avo::LineDashStyle p_dashStyle) -> void override
	{
		switch (p_dashStyle)
		{
			case Avo::LineDashStyle::Solid:
				m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_SOLID;
				break;
			case Avo::LineDashStyle::Dash:
				m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_DASH;
				break;
			case Avo::LineDashStyle::Dot:
				m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_DOT;
				break;
			case Avo::LineDashStyle::DashDot:
				m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_DASH_DOT;
				break;
			case Avo::LineDashStyle::DashDotDot:
				m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_DASH_DOT_DOT;
				break;
			case Avo::LineDashStyle::Custom:
				m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
		}
		updateStrokeStyle();
	}
	auto getLineDashStyle() -> Avo::LineDashStyle override
	{
		switch (m_strokeStyleProperties.dashStyle)
		{
			case D2D1_DASH_STYLE_SOLID:
				return Avo::LineDashStyle::Solid;
			case D2D1_DASH_STYLE_DASH:
				return Avo::LineDashStyle::Dash;
			case D2D1_DASH_STYLE_DOT:
				return Avo::LineDashStyle::Dot;
			case D2D1_DASH_STYLE_DASH_DOT:
				return Avo::LineDashStyle::DashDot;
			case D2D1_DASH_STYLE_DASH_DOT_DOT:
				return Avo::LineDashStyle::DashDotDot;
			case D2D1_DASH_STYLE_CUSTOM:
				return Avo::LineDashStyle::Custom;
		}
		return Avo::LineDashStyle::Unknown;
	}

	auto setLineDashOffset(float p_dashOffset) -> void override
	{
		m_strokeStyleProperties.dashOffset = p_dashOffset;
		updateStrokeStyle();
	}
	auto getLineDashOffset() -> float override
	{
		return m_strokeStyleProperties.dashOffset;
	}

	auto setLineDashCap(Avo::LineCap p_dashCap) -> void override
	{
		switch (p_dashCap)
		{
			case Avo::LineCap::Flat:
				m_strokeStyleProperties.dashCap = D2D1_CAP_STYLE_FLAT;
				break;
			case Avo::LineCap::Round:
				m_strokeStyleProperties.dashCap = D2D1_CAP_STYLE_ROUND;
				break;
			case Avo::LineCap::Square:
				m_strokeStyleProperties.dashCap = D2D1_CAP_STYLE_SQUARE;
				break;
			case Avo::LineCap::Triangle:
				m_strokeStyleProperties.dashCap = D2D1_CAP_STYLE_TRIANGLE;
		}
		updateStrokeStyle();
	}
	auto getLineDashCap() -> Avo::LineCap override
	{
		switch (m_strokeStyleProperties.dashCap)
		{
			case D2D1_CAP_STYLE_FLAT:
				return Avo::LineCap::Flat;
			case D2D1_CAP_STYLE_ROUND:
				return Avo::LineCap::Round;
			case D2D1_CAP_STYLE_SQUARE:
				return Avo::LineCap::Square;
			case D2D1_CAP_STYLE_TRIANGLE:
				return Avo::LineCap::Triangle;
		}
		return Avo::LineCap::Unknown;
	}

	//------------------------------

private:
	std::stack<bool> m_clipTypeStack;

	auto pushGeometryLayer(ID2D1Geometry* const p_geometry, float const p_opacity) -> void
	{
		m_context->PushLayer(
			D2D1::LayerParameters1(
				D2D1::InfiniteRect(), p_geometry, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
				D2D1::IdentityMatrix(), p_opacity, nullptr, 
				D2D1_LAYER_OPTIONS1_INITIALIZE_FROM_BACKGROUND | D2D1_LAYER_OPTIONS1_IGNORE_ALPHA // Improves performance :^)
			), nullptr
		);
		m_clipTypeStack.push(true);
	}
	
public:
	auto pushClipGeometry(Avo::Geometry const& p_geometry, float const p_opacity) -> void override
	{
		pushGeometryLayer(static_cast<Direct2dGeometry*>(p_geometry.getImplementation())->getGeometry(), p_opacity);
	}

	auto pushClipShape(Range<Avo::Point<> const*> const p_points, float const p_opacity) -> void override
	{
		if (!p_points.size())
		{
			return;
		}

		pushGeometryLayer(createPathGeometry(p_points, false, true), p_opacity);
	}
	auto popClipShape() -> void override
	{
		if (m_clipTypeStack.size())
		{
			if (m_clipTypeStack.top())
			{
				m_context->PopLayer();
			}
			else
			{
				m_context->PopAxisAlignedClip();
			}
			m_clipTypeStack.pop();
		}
	}

	auto pushClipRectangle(Avo::Rectangle<> const p_rectangle, float const p_opacity) -> void override
	{
		auto const rectangle = convertRectangleToRectf(p_rectangle);

		constexpr auto opaqueThreshold = 254./255.;
		if (p_opacity > opaqueThreshold)
		{
			m_context->PushAxisAlignedClip(rectangle, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			m_clipTypeStack.push(false);
		}
		else
		{
			auto geometry = ComReference<ID2D1RectangleGeometry>{};
			s_direct2DFactory->CreateRectangleGeometry(rectangle, &geometry);
			pushGeometryLayer(geometry, p_opacity);
		}
	}

	auto pushClipRectangle(
		Avo::Rectangle<> const p_rectangle, Avo::RectangleCorners const& p_corners, float const p_opacity
	) -> void override
	{
		pushGeometryLayer(createCornerRectanglePathGeometry(p_rectangle, p_corners, true), p_opacity);
	}

	auto pushRoundedClipRectangle(Avo::Rectangle<> const p_rectangle, float const p_radius, float const p_opacity) -> void override
	{
		auto geometry = ComReference<ID2D1RoundedRectangleGeometry>{};
		s_direct2DFactory->CreateRoundedRectangleGeometry(
			D2D1::RoundedRect(
				convertRectangleToRectf(p_rectangle),
				p_radius, p_radius
			), &geometry
		);

		pushGeometryLayer(geometry, p_opacity);
	}

	//------------------------------

private:
	template<typename Lambda>
	auto createShadowImage(
		Avo::Size<> const p_size, float p_blur, 
		Avo::Color const p_color, Lambda const& p_drawShape
	) -> Avo::Image
	{
		if (!p_size || !p_color.alpha) return {};

		p_blur *= 2.f/3.f;

		auto const targetLock = std::scoped_lock{m_targetMutex};

		auto dpiX = float{};
		auto dpiY = float{};
		m_context->GetDpi(&dpiX, &dpiY);
		auto const dipToPixelFactor = dpiX/USER_DEFAULT_SCREEN_DPI;

		auto targetBefore = ComReference<ID2D1Image>{};
		m_context->GetTarget(&targetBefore);
		m_context->SetDpi(USER_DEFAULT_SCREEN_DPI, USER_DEFAULT_SCREEN_DPI);

		auto inputBitmap = ComReference<ID2D1Bitmap1>{};
		m_context->CreateBitmap(
			D2D1::SizeU(p_size.width, p_size.height),
			nullptr, 0,
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			), &inputBitmap
		);

		m_context->SetTarget(inputBitmap);

		m_context->BeginDraw();
		p_drawShape();
		m_context->EndDraw();
		m_context->SetDpi(dpiX, dpiY);

		//------------------------------
		// Apply effect

		auto shadowEffect = ComReference<ID2D1Effect>{};
		m_context->CreateEffect(CLSID_D2D1Shadow, &shadowEffect);

		shadowEffect->SetInput(0, inputBitmap);
		shadowEffect->SetValue(
			D2D1_SHADOW_PROP_COLOR,
			D2D1::Vector4F(p_color.red, p_color.green, p_color.blue, p_color.alpha)
		);
		shadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, p_blur/dipToPixelFactor);

		//------------------------------
		// Convert to bitmap

		auto outputBitmap = static_cast<ID2D1Bitmap1*>(nullptr);
		auto const outputSize = Avo::Size<UINT32>{p_size + 6.f*p_blur};

		m_context->CreateBitmap(
			{outputSize.x, outputSize.y},
			0, outputSize.width*4,
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			), &outputBitmap
		);

		m_context->SetTarget(outputBitmap);
		m_context->BeginDraw();
		clear();
		m_context->DrawImage(shadowEffect, {p_blur*3.f/dipToPixelFactor, p_blur*3.f/dipToPixelFactor});
		m_context->EndDraw();
		m_context->SetTarget(targetBefore);

		return createImageFromImplementation(std::make_shared<Direct2dImage>(outputBitmap));
	}

public:
	auto createRectangleShadowImage(Avo::Size<> p_size, float p_blur, Avo::Color p_color) -> Avo::Image override
	{
		return createShadowImage(p_size, p_blur, p_color, [&]{ clear(0.f); });
	}
	auto createRectangleShadowImage(Avo::Size<> p_size, Avo::RectangleCorners const& p_corners, 
	                                float p_blur, Avo::Color p_color) -> Avo::Image override
	{
		return createShadowImage(p_size, p_blur, p_color, [&]{
			clear();
			setColor(0.f);
			fillRectangle(p_size, p_corners);
		});
	}
	auto createRoundedRectangleShadowImage(Avo::Size<> p_size, float p_radius, float p_blur, Avo::Color p_color) -> Avo::Image override
	{
		return createShadowImage(p_size, p_blur, p_color, [&]{
			clear();
			setColor(0.f);
			fillRoundedRectangle(p_size, p_radius);
		});
	}

	//------------------------------

	auto createImage(std::byte const* p_pixelData, Avo::Size<Avo::Pixels> p_size) -> Avo::Image override
	{
		auto bitmap = static_cast<ID2D1Bitmap1*>(nullptr);
		m_context->CreateBitmap(
			{static_cast<UINT32>(p_size.width), static_cast<UINT32>(p_size.height)},
			p_pixelData, p_size.width*4,
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS::D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			), &bitmap
		);
		return createImageFromImplementation(std::make_shared<Direct2dImage>(bitmap));
	}
	auto createImage(Avo::DataView p_imageData) -> Avo::Image override
	{
		auto stream = ComReference<IWICStream>{};
		s_imagingFactory->CreateStream(&stream);
		if (!stream) return {};

		// The stream won't modify p_imageData.
		stream->InitializeFromMemory(const_cast<BYTE*>(reinterpret_cast<BYTE const*>(p_imageData.data())), p_imageData.size());

		auto decoder = ComReference<IWICBitmapDecoder>{};
		s_imagingFactory->CreateDecoderFromStream(stream, 0, WICDecodeMetadataCacheOnDemand, &decoder);
		if (!decoder) return {};

		auto frame = ComReference<IWICBitmapFrameDecode>{};
		decoder->GetFrame(0, &frame);
		if (!frame) return {};

		auto formatConverter = ComReference<IWICFormatConverter>{};
		s_imagingFactory->CreateFormatConverter(&formatConverter);
		formatConverter->Initialize(
			frame, 
			GUID_WICPixelFormat32bppPBGRA, 
			WICBitmapDitherTypeNone, 
			nullptr, 0.f, 
			WICBitmapPaletteTypeMedianCut
		);

		auto bitmap = static_cast<ID2D1Bitmap1*>(nullptr);
		m_context->CreateBitmapFromWicBitmap(formatConverter, 0, &bitmap);
		if (!bitmap) return {};

		return createImageFromImplementation(std::make_shared<Direct2dImage>(bitmap));
	}
	auto createImage(std::string_view p_filePath) -> Avo::Image override
	{
		auto wideFilePath = std::array<char16, MAX_PATH>{};
		Avo::convertUtf8ToUtf16(p_filePath, wideFilePath);

		auto decoder = ComReference<IWICBitmapDecoder>{};
		s_imagingFactory->CreateDecoderFromFilename(
			reinterpret_cast<LPCWSTR>(wideFilePath.data()), 
			nullptr, GENERIC_READ, 
			WICDecodeMetadataCacheOnLoad, 
			&decoder
		);
		if (!decoder) return {};

		auto frame = ComReference<IWICBitmapFrameDecode>{};
		decoder->GetFrame(0, &frame);
		if (!frame) return {};

		auto formatConverter = ComReference<IWICFormatConverter>{};
		s_imagingFactory->CreateFormatConverter(&formatConverter);
		formatConverter->Initialize(
			frame, 
			GUID_WICPixelFormat32bppPBGRA, 
			WICBitmapDitherTypeNone, 
			nullptr, 0.f, 
			WICBitmapPaletteTypeMedianCut
		);
		if (!formatConverter) return {};

		auto bitmap = static_cast<ID2D1Bitmap1*>(nullptr);
		m_context->CreateBitmapFromWicBitmap(formatConverter, nullptr, &bitmap);
		if (!bitmap) return {};

		return createImageFromImplementation(std::make_shared<Direct2dImage>(bitmap));
	}
	auto createImageFromHandle(void* p_handle) -> Avo::Image override
	{
		auto wicBitmap = ComReference<IWICBitmap>{};

		if (auto result = s_imagingFactory->CreateBitmapFromHICON(static_cast<HICON>(p_handle), &wicBitmap);
		    result < 0)
		{
			wicBitmap = nullptr;
			s_imagingFactory->CreateBitmapFromHBITMAP(static_cast<HBITMAP>(p_handle), nullptr, WICBitmapAlphaChannelOption::WICBitmapUseAlpha, &wicBitmap);
		}

		auto formatConverter = ComReference<IWICFormatConverter>{};
		s_imagingFactory->CreateFormatConverter(&formatConverter);
		formatConverter->Initialize(wicBitmap, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
		if (!formatConverter) return {};

		auto bitmap = ComReference<ID2D1Bitmap1>{};
		m_context->CreateBitmapFromWicBitmap(formatConverter, &bitmap);
		if (!bitmap) return {};

		return createImageFromImplementation(std::make_shared<Direct2dImage>(bitmap));
	}

	//------------------------------

private:
	static auto getD2dBitmapFromImage(Avo::Image const& p_image) -> ID2D1Bitmap1*
	{
		return dynamic_cast<Direct2dImage*>(p_image.getImplementation())->getD2dBitmap();
	}

public:
	auto drawImage(Avo::Image const& p_image, float p_multiplicativeOpacity) -> void override
	{
		if (!p_image) return;

		auto const cropRectangle = p_image.getCropRectangle(),
		           innerBounds = p_image.getInnerBounds();

		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawBitmap( // SO MANY PARAMETERS
			getD2dBitmapFromImage(p_image),
			{innerBounds.left, innerBounds.top, innerBounds.right, innerBounds.bottom},
			p_image.getOpacity()*m_currentBrush->GetOpacity()*p_multiplicativeOpacity,
			p_image.getScalingMethod() == Avo::ImageScalingMethod::Pixelated ? 
				D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
			{cropRectangle.left, cropRectangle.top, cropRectangle.right, cropRectangle.bottom}
		);
	}

	//------------------------------

private:
	auto writeImageFileDataToStream(Avo::Image const& p_image, Avo::ImageFormat p_format, IStream* p_stream) -> bool
	{
		auto direct2dBitmap = getD2dBitmapFromImage(p_image);

		auto formatGuid = GUID{};
		switch (p_format)
		{
			case Avo::ImageFormat::Jpeg:
				formatGuid = GUID_ContainerFormatJpeg;
				break;
			case Avo::ImageFormat::Png:
				formatGuid = GUID_ContainerFormatPng;
				break;
			case Avo::ImageFormat::Bmp:
				formatGuid = GUID_ContainerFormatBmp;
				break;
			case Avo::ImageFormat::Ico:
				formatGuid = GUID_ContainerFormatIco;
		}

		auto bitmapEncoder = ComReference<IWICBitmapEncoder>{};
		s_imagingFactory->CreateEncoder(formatGuid, nullptr, &bitmapEncoder);
		if (!bitmapEncoder) return false;
		
		bitmapEncoder->Initialize(p_stream, WICBitmapEncoderNoCache);

		auto frameEncoder = ComReference<IWICBitmapFrameEncode>{};
		bitmapEncoder->CreateNewFrame(&frameEncoder, nullptr);
		if (!frameEncoder) return false;

		frameEncoder->Initialize(nullptr);

		auto device = ComReference<ID2D1Device>{};
		m_context->GetDevice(&device);
		if (!device) return false;

		auto imageEncoder = ComReference<IWICImageEncoder>{};
		s_imagingFactory->CreateImageEncoder(device, &imageEncoder);
		if (!imageEncoder) return false;

		imageEncoder->WriteFrame(direct2dBitmap, frameEncoder, nullptr);

		frameEncoder->Commit();
		bitmapEncoder->Commit();

		return true;
	}
	
public:
	auto createImageFileData(Avo::Image const& p_image, Avo::ImageFormat p_format) -> Avo::DataVector override
	{
		auto outputStream = ComReference{static_cast<IStream*>(createImageFileDataNativeStream(p_image, p_format))};

		STATSTG streamInfo;
		outputStream->Stat(&streamInfo, STATFLAG_NONAME);

		auto buffer = Avo::DataVector(streamInfo.cbSize.QuadPart);

		ULONG numberOfBytesWritten;
		outputStream->Seek({}, SEEK_SET, nullptr);
		outputStream->Read(buffer.data(), buffer.size(), &numberOfBytesWritten);

		if (numberOfBytesWritten < buffer.size())
		{
			buffer.resize(numberOfBytesWritten);
		}

		return buffer;
	}
	auto createImageFileDataNativeStream(Avo::Image const& p_image, Avo::ImageFormat p_format) -> void* override
	{
		if (auto outputStream = ComReference{SHCreateMemStream(0, 0)}; 
		    writeImageFileDataToStream(p_image, p_format, outputStream))
		{
			return outputStream.steal();
		}
		return {};
	}
	auto saveImageToFile(Avo::Image const& p_image, std::string_view p_filePath, Avo::ImageFormat p_format) -> bool override
	{
		auto filePath = std::array<char16, MAX_PATH>{};
		Avo::convertUtf8ToUtf16(p_filePath, filePath);
		
		auto outputStream = ComReference<IStream>{};
		SHCreateStreamOnFileW(reinterpret_cast<LPCWSTR>(filePath.data()), STGM_CREATE | STGM_WRITE, &outputStream);

		return writeImageFileDataToStream(p_image, p_format, outputStream);
	}

	//------------------------------

	auto createNativeImageFromImage(Avo::Image const& p_image) -> void* override
	{
		auto sourceBitmap = getD2dBitmapFromImage(p_image);
		auto size = sourceBitmap->GetPixelSize();

		auto cpuBitmap = ComReference<ID2D1Bitmap1>{};
		if (sourceBitmap->GetOptions() & D2D1_BITMAP_OPTIONS_CPU_READ)
		{
			cpuBitmap = sourceBitmap;
		}
		else
		{
			m_context->CreateBitmap(
				size, nullptr, 0, 
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, 
					sourceBitmap->GetPixelFormat()
				), 
				&cpuBitmap
			);
		}

		auto destinationPoint = D2D1_POINT_2U{0, 0};
		auto sourceRectangle = D2D1_RECT_U{0, 0, size.width, size.height};
		cpuBitmap->CopyFromBitmap(&destinationPoint, sourceBitmap, &sourceRectangle);

		// Pixel data
		auto mappedRectangle = D2D1_MAPPED_RECT{};
		cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mappedRectangle);

		auto bitmapInfoHeader = BITMAPINFOHEADER{};		
		bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapInfoHeader.biWidth = ceil(size.width/16.f)*16;
		bitmapInfoHeader.biHeight = -(long)size.height;
		bitmapInfoHeader.biBitCount = 32;
		bitmapInfoHeader.biCompression = BI_RGB;
		bitmapInfoHeader.biPlanes = 1;

		auto result = CreateDIBitmap(
			GetDC(static_cast<HWND>(m_window->getNativeHandle())), 
			&bitmapInfoHeader, CBM_INIT, mappedRectangle.bits,
			reinterpret_cast<BITMAPINFO*>(&bitmapInfoHeader), DIB_RGB_COLORS
		);

		cpuBitmap->Unmap();

		return result;
	}

	//------------------------------

private:
	auto createGradientStopCollection(Range<Avo::GradientStop*> p_stops)
	{
		auto gradientStops = std::vector<D2D1_GRADIENT_STOP>(p_stops.size());
		for (auto a : Indices{p_stops})
		{
			auto const& color = p_stops[a].color;
			gradientStops[a].color = {color.red, color.green, color.blue, color.alpha};
			gradientStops[a].position = p_stops[a].position;
		}

		auto stopCollection = ComReference<ID2D1GradientStopCollection>{};
		m_context->CreateGradientStopCollection(gradientStops.data(), p_stops.size(), &stopCollection);
		return stopCollection;
	}

public:
	auto createLinearGradient(Range<Avo::GradientStop*> p_gradientStops, 
	                          Avo::Point<> p_start, Avo::Point<> p_end) 
		-> Avo::LinearGradient override
	{
		auto stopCollection = createGradientStopCollection(p_gradientStops);

		auto brush = ComReference<ID2D1LinearGradientBrush>{};
		m_context->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties({p_start.x, p_start.y}, {p_end.x, p_end.y}),
			stopCollection, &brush
		);

		return createLinearGradientFromImplementation(std::make_shared<Direct2dLinearGradient>(brush));
	}
	auto createRadialGradient(Range<Avo::GradientStop*> p_gradientStops, 
	                          Avo::Point<> p_start, Avo::Point<> p_radius) 
		-> Avo::RadialGradient override
	{
		auto stopCollection = createGradientStopCollection(p_gradientStops);

		auto brush = ComReference<ID2D1RadialGradientBrush>{};
		m_context->CreateRadialGradientBrush(
			D2D1::RadialGradientBrushProperties({p_start.x, p_start.y}, {}, p_radius.x, p_radius.y), 
			stopCollection, &brush
		);

		return createRadialGradientFromImplementation(std::make_shared<Direct2dRadialGradient>(brush));
	}

	auto setGradient(Avo::LinearGradient const& p_gradient) -> void override
	{
		m_currentBrush = dynamic_cast<Direct2dLinearGradient*>(p_gradient.getImplementation())->getD2dBrush();
	}
	auto setGradient(Avo::RadialGradient const& p_gradient) -> void override
	{
		m_currentBrush = dynamic_cast<Direct2dRadialGradient*>(p_gradient.getImplementation())->getD2dBrush();
	}

	auto setColor(Avo::Color p_color) -> void override
	{
		m_solidColorBrush->SetColor({p_color.red, p_color.green, p_color.blue, p_color.alpha});
		m_currentBrush = m_solidColorBrush;
	}
	auto setOpacity(float p_opacity) -> void override
	{
		m_brushOpacity = p_opacity;
	}

	//------------------------------

private:
	ComReference<IDWriteFontCollection> m_fontCollection;
	std::vector<FontData> m_fontData;

	auto updateFontCollection() -> void
	{
		m_fontCollection = nullptr; // Releases, important.
		s_directWriteFactory->CreateCustomFontCollection(s_fontCollectionLoader, &m_fontData, sizeof(m_fontData), &m_fontCollection);
	}

public:
	auto addFont(std::string_view p_filePath) -> void override
	{
		m_fontData.emplace_back(std::make_shared<Avo::DataVector>(Avo::readFile(p_filePath)));
		updateFontCollection();
	}

	auto addFont(Avo::DataVector&& p_data) -> void override
	{
		m_fontData.emplace_back(std::make_shared<Avo::DataVector>(std::move(p_data)));
		updateFontCollection();
	}
	auto addFont(Avo::DataView p_data) -> void override
	{
		m_fontData.emplace_back(std::make_shared<Avo::DataVector>(p_data.data(), p_data.data() + p_data.size()));
		updateFontCollection();
	}

	//------------------------------

private:
	ComReference<IDWriteTextFormat> m_textFormat;
	static constexpr auto MAX_FONT_FAMILY_NAME_SIZE = 200;
public:
	auto setDefaultTextProperties(Avo::TextProperties const& p_textProperties) -> void override
	{
		auto fontFamily = std::array<char16, MAX_FONT_FAMILY_NAME_SIZE>{};
		Avo::convertUtf8ToUtf16(p_textProperties.fontFamilyName, fontFamily);

		m_textFormat = nullptr; // Releases, important.
		s_directWriteFactory->CreateTextFormat(
			reinterpret_cast<WCHAR const*>(fontFamily.data()), 
			m_fontCollection, 
			static_cast<DWRITE_FONT_WEIGHT>(p_textProperties.fontWeight),
			static_cast<DWRITE_FONT_STYLE>(p_textProperties.fontStyle), 
			static_cast<DWRITE_FONT_STRETCH>(p_textProperties.fontStretch),
			p_textProperties.fontSize, L"", &m_textFormat
		);
		m_textFormat->SetTextAlignment(DirectWriteText::convertTextAlignToDirectWrite(p_textProperties.textAlign));
		m_textFormat->SetReadingDirection(DirectWriteText::convertReadingDirectionToDirectWrite(p_textProperties.readingDirection));
		m_textFormat->SetLineSpacing(
			DWRITE_LINE_SPACING_METHOD_PROPORTIONAL, 
			p_textProperties.lineHeight, p_textProperties.lineHeight*0.8f
		);
		m_textProperties = p_textProperties;
	}
	auto getDefaultTextProperties() -> Avo::TextProperties override
	{
		return m_textProperties;
	}

	//------------------------------

private:
	auto getDWriteTextLayoutFromText(Avo::Text const& p_text) -> IDWriteTextLayout1*
	{
		return dynamic_cast<DirectWriteText*>(p_text.getImplementation())->getDWriteTextLayout();
	}

public:
	auto createText(std::string_view p_string, float p_fontSize, Avo::Rectangle<> p_bounds) -> Avo::Text override
	{
		p_bounds.clipNegativeSpace();

		auto wideString = Avo::convertUtf8ToUtf16(p_string);

		auto textLayout = static_cast<IDWriteTextLayout1*>(nullptr);
		s_directWriteFactory->CreateTextLayout( // So many parameters...
			reinterpret_cast<WCHAR const*>(wideString.data()), wideString.size(), 
			m_textFormat, 
			p_bounds.getWidth(), p_bounds.getHeight(), 
			reinterpret_cast<IDWriteTextLayout**>(&textLayout)
		);
		
		auto textRange = DWRITE_TEXT_RANGE{0, static_cast<UINT32>(wideString.size())};
		textLayout->SetFontSize(p_fontSize, textRange);
		textLayout->SetCharacterSpacing(
			m_textProperties.characterSpacing/2, 
			m_textProperties.characterSpacing/2, 
			0.f, textRange
		);

		return createTextFromImplementation(std::make_shared<DirectWriteText>(textLayout, wideString, p_string, p_bounds));
	}
	auto drawText(Avo::Text const& p_text) -> void override
	{
		if (!p_text)
		{
			return;
		}
		auto* const textLayout = getDWriteTextLayoutFromText(p_text);

		auto name = std::array<WCHAR, MAX_FONT_FAMILY_NAME_SIZE>{};
		textLayout->GetFontFamilyName(name.data(), name.size());

		auto overhangMetrics = DWRITE_OVERHANG_METRICS{};
		if (p_text.getIsTopTrimmed())
		{
			textLayout->GetOverhangMetrics(&overhangMetrics);
		}
		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawTextLayout(
			D2D1::Point2F(p_text.getLeft(), p_text.getTop() + overhangMetrics.top),
			textLayout, m_currentBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
		);
	}
	auto drawText(std::string_view p_string, Avo::Rectangle<> p_rectangle) -> void override
	{
		if (p_string == "") return;

		auto wideString = Avo::convertUtf8ToUtf16(p_string);

		m_currentBrush->SetOpacity(m_brushOpacity);
		m_context->DrawTextW(
			reinterpret_cast<WCHAR const*>(wideString.data()), wideString.size(), 
			m_textFormat, convertRectangleToRectf(p_rectangle),
			m_currentBrush, D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_NONE
		);
	}
	auto drawText(std::string_view p_string, Avo::Point<> p_position) -> void override
	{
		drawText(p_string, Avo::Rectangle{p_position, convertSizefToSize(m_context->GetSize())*2});
	}

	//------------------------------

private:
	/*
		This is all pretty hard to follow. It's DirectX's fault. Such a cluttered API. 
		Maybe also because of their use of COM interfaces which are a painful blend of C and C++.
	*/
	auto createSwapChainAndDrawingContext(float p_dpi) -> void
	{
		/*
			Create the Direct3D device, which will be used later to create the Direct2D drawing 
			context used for drawing as well as the swap chain used for swapping front and back 
			buffers and acts as DirectX's interface to the window.
		*/
		auto d3dDevice = ComReference<ID3D11Device>{};
		{
			auto d3dDeviceContext = ComReference<ID3D11DeviceContext>{};

			auto featureLevels = std::array{
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3,
				D3D_FEATURE_LEVEL_9_2,
				D3D_FEATURE_LEVEL_9_1,
			};
			auto featureLevel = D3D_FEATURE_LEVEL{};
			D3D11CreateDevice(
				0,
				D3D_DRIVER_TYPE_HARDWARE,
				0,
				D3D11_CREATE_DEVICE_BGRA_SUPPORT
	#ifdef _DEBUG 
				| D3D11_CREATE_DEVICE_DEBUG
	#endif
				,
				featureLevels.data(),
				featureLevels.size(),
				D3D11_SDK_VERSION,
				&d3dDevice,
				&featureLevel,
				&d3dDeviceContext
			);
		}


		//------------------------------
		// Get dxgi factory from the Direct3D device.
		// Both of those will be used afterwards to create the dxgi swap chain.

		auto dxgiFactory = ComReference<IDXGIFactory2>{};
		{
			/*
				First we obtain the DXGI device from the Direct3D device.
				This will first be used to create a Direct2D device, which 
				in turn is used to create our Direct2D drawing context.
				Afterwards, we obtain the DXGI factory from the DXGI device,
				which will be needed next outside this scope to create the 
				DXGI swap chain.
			*/
			
			auto dxgiDevice = ComReference<IDXGIDevice1>{};
			d3dDevice->QueryInterface(&dxgiDevice);
			dxgiDevice->SetMaximumFrameLatency(1U);

			{
				// Create the Direct2D device and context.
				auto direct2DDevice = ComReference<ID2D1Device1>{}; 
				s_direct2DFactory->CreateDevice(dxgiDevice, &direct2DDevice);
				direct2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &m_context);
				m_context->SetDpi(p_dpi, p_dpi);
			}

			// Obtain DXGI factory from DXGI device.
			auto dxgiAdapter = ComReference<IDXGIAdapter>{};
			dxgiDevice->GetAdapter(&dxgiAdapter);
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
		}

		// We don't want DXGI changing the fullscreen state using its own key bindings...
		auto windowHandle = static_cast<HWND>(m_window->getNativeHandle());
		dxgiFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_WINDOW_CHANGES);

		//------------------------------
		// Create swap chain, which holds the front and back buffers and is connected to the window.

		auto size = Avo::Size<UINT>{m_window->getSize()*p_dpi/USER_DEFAULT_SCREEN_DPI};
		auto swapChainDescription = DXGI_SWAP_CHAIN_DESC1{
			/*.Width = */size.width,
			/*.Height = */size.height,
			/*.Format = */DXGI_FORMAT_B8G8R8A8_UNORM,
			/*.Stereo = */false,
			/*.SampleDesc = */{1, 0},
			/*.BufferUsage = */DXGI_USAGE_RENDER_TARGET_OUTPUT,
			/*.BufferCount = */2,
			/*.Scaling = */DXGI_SCALING::DXGI_SCALING_NONE,
			/*.SwapEffect = */DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
		};

		dxgiFactory->CreateSwapChainForHwnd(
			d3dDevice, windowHandle,
			&swapChainDescription, nullptr,
			nullptr, &m_swapChain
		);
	}
	auto initializeContext() -> void
	{
		float dpi = GetDpiForSystem();

		createSwapChainAndDrawingContext(dpi);

		//------------------------------
		// Initialize the background color of the swap chain's buffers.

		auto color = DXGI_RGBA{0.5f, 0.5f, 0.5f, 1.f};
		m_swapChain->SetBackgroundColor(&color);

		//------------------------------
		// Create a target bitmap which is connected to the back buffer of the window.

		{
			auto lock = Direct2dLock{};
			
			ComReference<IDXGISurface> dxgiBackBuffer;
			m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));

			createTargetWindowBitmapFromDxgiBackBuffer(dxgiBackBuffer, dpi);
		}

		m_context->SetTarget(m_targetWindowBitmap);
	}
	auto initializeFonts() -> void
	{
		auto addData = [&](auto data, auto size) {
			auto const start = reinterpret_cast<std::byte const*>(data);
			m_fontData.emplace_back(std::make_shared<Avo::DataVector>(start, start + size));
		};

		m_fontData.reserve(8);
		addData(FONT_DATA_ROBOTO_LIGHT, FONT_DATA_SIZE_ROBOTO_LIGHT);
		addData(FONT_DATA_ROBOTO_REGULAR, FONT_DATA_SIZE_ROBOTO_REGULAR);
		addData(FONT_DATA_ROBOTO_MEDIUM, FONT_DATA_SIZE_ROBOTO_MEDIUM);
		addData(FONT_DATA_ROBOTO_BOLD, FONT_DATA_SIZE_ROBOTO_BOLD);
		addData(FONT_DATA_MATERIAL_ICONS, FONT_DATA_SIZE_MATERIAL_ICONS);
		updateFontCollection();

		setDefaultTextProperties(m_textProperties);

		// Just for debugging...
		//std::vector<wchar_t*> fontFamilyNames;
		//for (uint32 a = 0; a < m_fontCollection->GetFontFamilyCount(); a++)
		//{
		//	IDWriteFontFamily* fontFamily;
		//	m_fontCollection->GetFontFamily(a, &fontFamily);
		//	IDWriteLocalizedStrings* names;
		//	fontFamily->GetFamilyNames(&names);
		//	wchar_t* buffer = new wchar_t[30];
		//	names->GetString(0, buffer, 30);
		//	fontFamilyNames.push_back(buffer);
		//}

		//m_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE::D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
	}
	auto initializeDrawing() -> void
	{
		// Initial color doesn't matter, it'll be changed afterwards.
		m_context->CreateSolidColorBrush({}, &m_solidColorBrush);
		m_currentBrush = m_solidColorBrush;

		m_strokeStyleProperties.dashCap = D2D1_CAP_STYLE_FLAT;
		m_strokeStyleProperties.dashOffset = 1.f;
		m_strokeStyleProperties.dashStyle = D2D1_DASH_STYLE_SOLID;
		m_strokeStyleProperties.lineJoin = D2D1_LINE_JOIN_ROUND;
		m_strokeStyleProperties.miterLimit = 0.f;
		m_strokeStyleProperties.startCap = D2D1_CAP_STYLE_FLAT;
		m_strokeStyleProperties.endCap = D2D1_CAP_STYLE_FLAT;
		m_strokeStyleProperties.transformType = D2D1_STROKE_TRANSFORM_TYPE::D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
		updateStrokeStyle();
	}

public:
	Direct2dDrawingContext(Avo::Window* p_window) :
		m_window{p_window}
	{
		initializeContext();
		
		initializeDrawing();

		initializeFonts();
	}
	~Direct2dDrawingContext()
	{
		destroyStaticResources();
	}
};
ComReference<ID2D1Factory2> Direct2dDrawingContext::s_direct2DFactory;
ComReference<IDWriteFactory1> Direct2dDrawingContext::s_directWriteFactory;
ComReference<FontCollectionLoader> Direct2dDrawingContext::s_fontCollectionLoader;
ComReference<FontFileLoader> Direct2dDrawingContext::s_fontFileLoader;
ComReference<IWICImagingFactory2> Direct2dDrawingContext::s_imagingFactory;
#endif
#ifdef __linux__

bool areOpenGlFunctionsLoaded = false;

PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM3IPROC glUniform3i;
PFNGLUNIFORM4IPROC glUniform4i;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;

//------------------------------

class OpenGlShader
{
private:
	GLuint m_programID{0};
	GLuint m_vertexArrayObjectID{0};

	GLuint m_vertexBufferID{0};
	uint32 m_vertexBufferSize{0};
	uint32 m_vertexSize{0};

	GLuint m_indexBufferID{0};
	uint32 m_indexBufferSize{0};

public:
	OpenGlShader() = default;
	~OpenGlShader()
	{
		if (m_programID)
		{
			glDeleteProgram(m_programID);
		}
	}

	void compile(char const* p_vertexShaderSource, char const* p_fragmentShaderSource)
	{
		GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShaderID, 1, &p_vertexShaderSource, nullptr);
		glCompileShader(vertexShaderID);

		GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShaderID, 1, &p_fragmentShaderSource, nullptr);
		glCompileShader(fragmentShaderID);

#ifdef _DEBUG
		GLint wasSuccessful = 1;

		// Check for vertex shader compile errors
		glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &wasSuccessful);
		if (!wasSuccessful)
		{
			GLchar message[256];
			glGetShaderInfoLog(vertexShaderID, 256, nullptr, message);
			Console::println("\nVertex shader compile error(s)!\n", message);
		}

		// Check for fragment shader compile errors
		glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &wasSuccessful);
		if (!wasSuccessful)
		{
			GLchar message[256];
			glGetShaderInfoLog(fragmentShaderID, 256, nullptr, message);
			Console::println("\nFragment shader compile error(s)!\n", message);
		}
#endif

		m_programID = glCreateProgram();
		glAttachShader(m_programID, vertexShaderID);
		glAttachShader(m_programID, fragmentShaderID);
		glLinkProgram(m_programID);

		glDeleteShader(fragmentShaderID);
		glDeleteShader(vertexShaderID);

		m_vertexArrayObjectID = 0;
		glGenVertexArrays(1, &m_vertexArrayObjectID);

		m_vertexBufferID = 0;
		glGenBuffers(1, &m_vertexBufferID);
	}

	void setVertexData(float* p_data, uint32 p_dataSize, uint32 p_vertexSize, GLenum p_usage = GL_STATIC_DRAW)
	{
        glBindVertexArray(m_vertexArrayObjectID);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, p_dataSize, p_data, p_usage);

		m_vertexBufferSize = p_dataSize;
		m_vertexSize = p_vertexSize;
	}
	/*
		Sets information about a vertex shader input.
		p_shaderInputIndex is the layout index of the "in" variable in the vertex shader.
		p_numberOfComponentsInShaderInput is the number of floats stored in the "in" variable, and is either 1, 2, 3 or 4.
		p_pointerPosition is the start position of the input in the vertex buffer, in number of floats.
	*/
	void configureVertexShaderInput(uint32 p_shaderInputIndex, uint32 p_numberOfComponentsInShaderInput, uint32 p_pointerPosition)
	{
		glBindVertexArray(m_vertexArrayObjectID);

		glVertexAttribPointer(p_shaderInputIndex, p_numberOfComponentsInShaderInput, GL_FLOAT, GL_FALSE, m_vertexSize, (void*)(p_pointerPosition*sizeof(float)));
		glEnableVertexAttribArray(p_shaderInputIndex);
	}

	void setVertexDataOrder(uint32* p_indices, uint32 p_indexCount, GLenum p_usage = GL_STATIC_DRAW)
	{
        glBindVertexArray(m_vertexArrayObjectID);

	    glGenBuffers(1, &m_indexBufferID);
	    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID);
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, p_indexCount, p_indices, p_usage);

	    m_indexBufferSize = p_indexCount;
	}

    void setUniformValue(char const* p_name, float p_value)
	{
		glUniform1f(glGetUniformLocation(m_programID, p_name), p_value);
	}
	void setUniformVector(char const* p_name, float p_value_0, float p_value_1)
	{
		glUniform2f(glGetUniformLocation(m_programID, p_name), p_value_0, p_value_1);
	}
	void setUniformVector(char const* p_name, float p_value_0, float p_value_1, float p_value_2)
	{
		glUniform3f(glGetUniformLocation(m_programID, p_name), p_value_0, p_value_1, p_value_2);
	}
	void setUniformVector(char const* p_name, float p_value_0, float p_value_1, float p_value_2, float p_value_3)
	{
		glUniform4f(glGetUniformLocation(m_programID, p_name), p_value_0, p_value_1, p_value_2, p_value_3);
	}
	void setUniformValue(char const* p_name, int p_value)
	{
		glUniform1i(glGetUniformLocation(m_programID, p_name), p_value);
	}
	void setUniformVector(char const* p_name, int p_value_0, int p_value_1)
	{
		glUniform2i(glGetUniformLocation(m_programID, p_name), p_value_0, p_value_1);
	}
	void setUniformVector(char const* p_name, int p_value_0, int p_value_1, int p_value_2)
	{
		glUniform3i(glGetUniformLocation(m_programID, p_name), p_value_0, p_value_1, p_value_2);
	}
	void setUniformVector(char const* p_name, int p_value_0, int p_value_1, int p_value_2, int p_value_3)
	{
		glUniform4i(glGetUniformLocation(m_programID, p_name), p_value_0, p_value_1, p_value_2, p_value_3);
	}
    void setUniformColor(char const* p_name, Avo::Color p_color)
    {
		glUniform4f(glGetUniformLocation(m_programID, p_name), p_color.red, p_color.green, p_color.blue, p_color.alpha);
	}
	/*
		Sets a matrix uniform with 2 rows and 3 columns.
		The values are in column-major order.
	*/
	void setUniformMatrix3x3(char const* p_name, float* p_values)
	{
		glUniformMatrix3fv(glGetUniformLocation(m_programID, p_name), 1, GL_FALSE, p_values);
	}

	void use()
	{
		glUseProgram(m_programID);
	}

    /*
		If p_numberOfVerticesToRender is -1, it renders all vertices or all referred to by a previous call to setVertexDataOrder.
    */
    void draw(uint32 p_startVertexIndex = 0, uint32 p_numberOfVerticesToDraw = -1, GLenum p_mode = GL_TRIANGLES)
    {
	    glBindVertexArray(m_vertexArrayObjectID);

        if (m_indexBufferID)
        {
            glDrawElements(p_mode, p_numberOfVerticesToDraw == -1 ? m_indexBufferSize : p_numberOfVerticesToDraw, GL_UNSIGNED_INT, nullptr);
		}
        else
        {
            glDrawArrays(p_mode, p_startVertexIndex, p_numberOfVerticesToDraw == -1 ? m_vertexBufferSize/m_vertexSize : p_numberOfVerticesToDraw);
		}
	}
};

//------------------------------

class ClippingShape
{
private:
	Avo::Rectangle<> m_bounds;
	
public:
	explicit ClippingShape(Avo::Rectangle<> p_bounds) :
		m_bounds(p_bounds)
	{
	}
	ClippingShape(float p_left, float p_top, float p_right, float p_bottom) :
		m_bounds(p_left, p_top, p_right, p_bottom)
	{
	}

	Avo::Rectangle<> getBounds()
	{
		return m_bounds;
	}
};

class OpenGlDrawCall
{
public:
	uint32 vertexBufferStartIndex{0};
	uint32 numberOfVertices{0};
	GLenum primitiveType{GL_TRIANGLES};
	Avo::Color color;
	int renderMode{0};
	float lineWidth{1.f};
	GLuint texture{0u};
	float transform[9];
};

class OpenGlGeometry : public Avo::Geometry
{
public:
	std::vector<float> vertexBuffer;
	bool isStroked{false};
};

class OpenGlImage : public Avo::Image
{
private:
	GLuint m_textureID;
	Avo::Point<uint32> m_originalSize;

	Avo::ImageScalingMethod m_scalingMethod{Avo::ImageScalingMethod::Pixelated};
	Avo::ImageBoundsSizing m_boundsSizing{Avo::ImageBoundsSizing::Contain};
	Avo::Point<> m_boundsPositioning{0.5f, 0.5f};

	Avo::Rectangle<> m_cropRectangle;
	float m_opacity{1.f};

public:
	OpenGlImage(GLuint p_textureID, uint32 p_width, uint32 p_height, float p_dipToPixelFactor) :
		m_textureID(p_textureID), m_originalSize(p_width, p_height)
	{
		m_bounds.set(0.f, 0.f, p_width*p_dipToPixelFactor, p_height*p_dipToPixelFactor);
		m_cropRectangle = m_bounds;
	}

	//------------------------------

	void setCropRectangle(Avo::Rectangle<> p_rectangle) override
	{
		m_cropRectangle = p_rectangle;
	}
	Avo::Rectangle<> getCropRectangle() const override
	{
		return m_cropRectangle;
	}

	Avo::Point<uint32> getOriginalSize() const override
	{
		return m_originalSize;
	}
	uint32 getOriginalWidth() const override
	{
		return m_originalSize.x;
	}
	uint32 getOriginalHeight() const override
	{
		return m_originalSize.y;
	}

	//------------------------------

	void setBoundsSizing(Avo::ImageBoundsSizing p_sizeMode) override
	{
		m_boundsSizing = p_sizeMode;
	}
	Avo::ImageBoundsSizing getBoundsSizing() const override
	{
		return m_boundsSizing;
	}

	void setBoundsPositioning(float p_x, float p_y) override
	{
		m_boundsPositioning.set(p_x, p_y);
	}
	void setBoundsPositioningX(float p_x) override
	{
		m_boundsPositioning.x = p_x;
	}
	void setBoundsPositioningY(float p_y) override
	{
		m_boundsPositioning.y = p_y;
	}
	Avo::Point<> getBoundsPositioning() const override
	{
		return m_boundsPositioning;
	}
	float getBoundsPositioningX() const override
	{
		return m_boundsPositioning.x;
	}
	float getBoundsPositioningY() const override
	{
		return m_boundsPositioning.x;
	}

	//------------------------------

	void setScalingMethod(Avo::ImageScalingMethod p_scalingMethod) override
	{
		m_scalingMethod = p_scalingMethod;
	}
	Avo::ImageScalingMethod getScalingMethod() const override
	{
		return m_scalingMethod;
	}

	//------------------------------

	void setOpacity(float p_opacity) override
	{
		m_opacity = p_opacity;
	}
	float getOpacity() const override
	{
		return m_opacity;
	}

	//------------------------------

	float getInnerWidth() const override
	{
		if (m_boundsSizing != Avo::ImageBoundsSizing::Stretch &&
		    m_boundsSizing == Avo::ImageBoundsSizing::Contain != m_bounds.getWidth()/m_bounds.getHeight() < m_originalSize.x/(float)m_originalSize.y)
		{
			return m_bounds.getHeight()*m_originalSize.x/(float)m_originalSize.y;
		}
		return m_bounds.getWidth();
	}
	float getInnerHeight() const override
	{
		if (m_boundsSizing != Avo::ImageBoundsSizing::Stretch &&
		    m_boundsSizing == Avo::ImageBoundsSizing::Contain != m_bounds.getWidth()/m_bounds.getHeight() > m_originalSize.x/(float)m_originalSize.y)
		{
			return m_bounds.getWidth()*m_originalSize.y/(float)m_originalSize.x;
		}
		return m_bounds.getHeight();
	}
	Avo::Point<> getInnerSize() const override
	{
		return Avo::Point<>(getInnerWidth(), getInnerHeight());
	}
	Avo::Rectangle<> getInnerBounds() const override
	{
		if (m_boundsSizing == Avo::ImageBoundsSizing::Stretch)
		{
			return m_bounds;
		}

		Avo::Rectangle<> innerBounds = m_bounds;

		bool areBoundsProportionallyWider = m_bounds.getWidth()/m_bounds.getHeight() > m_originalSize.x/(float)m_originalSize.y;
		if (m_boundsSizing == Avo::ImageBoundsSizing::Fill != areBoundsProportionallyWider)
		{
			innerBounds.setWidth(m_bounds.getHeight()*m_originalSize.x/(float)m_originalSize.y);
		}
		else if (m_boundsSizing == Avo::ImageBoundsSizing::Contain != areBoundsProportionallyWider)
		{
			innerBounds.setHeight(m_bounds.getWidth()*m_originalSize.y/(float)m_originalSize.x);
		}

		innerBounds.move(m_boundsPositioning.x*(m_bounds.getWidth() - innerBounds.getWidth()), m_boundsPositioning.y*(m_bounds.getHeight() - innerBounds.getHeight()));

		return innerBounds;
	}

	void* getHandle() const override
	{
		return (void*)&m_textureID;
	}
};

class OpenGlDrawingContext :
	public Avo::DrawingContext
{
private:
	static constexpr uint32 CIRCLE_RESOLUTION{50};
	static constexpr uint32 VERTEX_DATA_LENGTH{8};

	static bool s_areStaticVariablesInitialized;
	static Avo::Point<> s_unitCirclePoints[CIRCLE_RESOLUTION + 1];

	static void initializeStaticVariables()
	{
		if (s_areStaticVariablesInitialized)
		{
			return;
		}

		s_unitCirclePoints[0].set(1.f, 0.f);
		s_unitCirclePoints[CIRCLE_RESOLUTION].set(1.f, 0.f);
		for (uint32 a = 1; a < CIRCLE_RESOLUTION; a++)
		{
			s_unitCirclePoints[a].setPolar((float)Avo::TAU*a/(float)CIRCLE_RESOLUTION);
		}

		s_areStaticVariablesInitialized = true;
	}

private:
	LinuxWindow* m_window;
	XWindow m_windowHandle;
	Display* m_server;
	GLXContext m_context;
	Avo::Color m_backgroundColor;
	
	float m_transformMatrix[9]{};
	Avo::Point<> m_scale{1.f, 1.f};

	Avo::Color m_currentColor;
	OpenGlShader m_renderShader;
	std::vector<OpenGlDrawCall> m_drawCalls;
	std::vector<float> m_vertexBuffer;

	std::stack<ClippingShape> m_clippingShapeStack;
	Avo::LineJoin m_lineJoin{Avo::LineJoin::Miter};
	Avo::LineCap m_lineCap{Avo::LineCap::Round};

	Avo::Point<> m_size;

	//------------------------------

	static void loadOpenGlFunctions()
	{
		if (areOpenGlFunctionsLoaded)
		{
			return;
		}
		
		__GLXextFuncPtr function = nullptr;
#define load(p_name) function = glXGetProcAddressARB((GLubyte const*)(#p_name)); memcpy(&p_name, &function, sizeof(p_name))

		load(glXCreateContextAttribsARB);
		load(glGenBuffers);
		load(glBindBuffer);
		load(glBufferData);
		load(glCreateShader);
		load(glShaderSource);
		load(glCompileShader);
		load(glGetShaderiv);
		load(glGetShaderInfoLog);
		load(glCreateProgram);
		load(glAttachShader);
		load(glLinkProgram);
		load(glDeleteProgram);
		load(glDeleteShader);
		load(glVertexAttribPointer);
		load(glEnableVertexAttribArray);
		load(glGenVertexArrays);
		load(glBindVertexArray);
		load(glUseProgram);
		load(glGetUniformLocation);
		load(glUniform1f);
		load(glUniform2f);
		load(glUniform3f);
		load(glUniform4f);
		load(glUniform1i);
		load(glUniform2i);
		load(glUniform3i);
		load(glUniform4i);
		load(glUniformMatrix3fv);
		load(glGenerateMipmap);

#undef load

		areOpenGlFunctionsLoaded = true;
	}
	/*
		This is called after the vertices have been added to the buffer.
	*/
	void addDrawCall(uint32 p_numberOfVertices, GLuint p_texture = 0u, GLenum p_primitiveType = GL_TRIANGLES, float p_opacity = 1.f)
	{
		OpenGlDrawCall drawCall;
		drawCall.vertexBufferStartIndex = uint32(m_vertexBuffer.size()/VERTEX_DATA_LENGTH - p_numberOfVertices);
		drawCall.numberOfVertices = p_numberOfVertices;
		drawCall.primitiveType = p_primitiveType;
		drawCall.color = {m_currentColor, p_opacity};
		drawCall.renderMode = bool(p_texture) + 1;
		drawCall.texture = p_texture;
		m_drawCalls.push_back(drawCall);
		memcpy(m_drawCalls.back().transform, m_transformMatrix, sizeof(m_transformMatrix));
	}
	/*
		This is called after the vertices have been added to the buffer.
	*/
	void addDrawCall(uint32 p_numberOfVertices, float p_lineWidth, GLenum p_primitiveType = GL_TRIANGLES)
	{
		OpenGlDrawCall drawCall;
		drawCall.vertexBufferStartIndex = uint32(m_vertexBuffer.size()/VERTEX_DATA_LENGTH - p_numberOfVertices);
		drawCall.numberOfVertices = p_numberOfVertices;
		drawCall.primitiveType = p_primitiveType;
		drawCall.color = m_currentColor;
		drawCall.renderMode = 0;
		drawCall.lineWidth = p_lineWidth;
		m_drawCalls.push_back(drawCall);
		memcpy(m_drawCalls.back().transform, m_transformMatrix, sizeof(m_transformMatrix));
	}

	static uint32 getCirclePointStrideForRadiusSquared(float p_radiusSquared)
	{
		return p_radiusSquared <= 25.f*25.f ? (p_radiusSquared <= 5.f*5.f ? 4 : 2) : 1;
	}
	/*
		The primitive type is GL_TRIANGLES
	*/
	static void createStrokedCornerRectangleGeometry(std::vector<float>& p_buffer, float p_left, float p_top, float p_right, float p_bottom, Avo::RectangleCorners p_corners)
	{
		if (p_right - p_left < p_corners.topLeftSizeX + p_corners.topRightSizeY)
		{
			float factor = (p_right - p_left)/(p_corners.topLeftSizeX + p_corners.topRightSizeX);
			p_corners.topLeftSizeX *= factor;
			p_corners.topRightSizeX *= factor;
		}
		if (p_right - p_left < p_corners.bottomLeftSizeX + p_corners.bottomRightSizeY)
		{
			float factor = (p_right - p_left)/(p_corners.bottomLeftSizeX + p_corners.bottomRightSizeX);
			p_corners.bottomLeftSizeX *= factor;
			p_corners.bottomRightSizeX *= factor;
		}
		if (p_bottom - p_top < p_corners.topLeftSizeX + p_corners.bottomLeftSizeY)
		{
			float factor = (p_bottom - p_top)/(p_corners.topLeftSizeX + p_corners.bottomLeftSizeY);
			p_corners.topLeftSizeX *= factor;
			p_corners.bottomLeftSizeX *= factor;
		}
		if (p_bottom - p_top < p_corners.topRightSizeX + p_corners.bottomRightSizeY)
		{
			float factor = (p_bottom - p_top)/(p_corners.topRightSizeX + p_corners.bottomRightSizeY);
			p_corners.topRightSizeX *= factor;
			p_corners.bottomRightSizeX *= factor;
		}

		p_buffer.insert(
			p_buffer.end(),
			{
				p_left + p_corners.topLeftSizeX  , p_top, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f,
				p_right - p_corners.topRightSizeX, p_top, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f,
				p_right - p_corners.topRightSizeX, p_top, 0.f, 1.f , 1.f, 0.f, 0.f, 0.f,
				p_right - p_corners.topRightSizeX, p_top, 0.f, 1.f , 1.f, 0.f, 0.f, 0.f,
				p_left + p_corners.topLeftSizeX  , p_top, 0.f, 1.f , 1.f, 0.f, 0.f, 0.f,
				p_left + p_corners.topLeftSizeX  , p_top, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f,

				p_left + p_corners.bottomLeftSizeX  , p_bottom, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f,
				p_right - p_corners.bottomRightSizeX, p_bottom, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f,
				p_right - p_corners.bottomRightSizeX, p_bottom, 0.f, 1.f , 1.f, 0.f, 0.f, 0.f,
				p_right - p_corners.bottomRightSizeX, p_bottom, 0.f, 1.f , 1.f, 0.f, 0.f, 0.f,
				p_left + p_corners.bottomLeftSizeX  , p_bottom, 0.f, 1.f , 1.f, 0.f, 0.f, 0.f,
				p_left + p_corners.bottomLeftSizeX  , p_bottom, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f,

				p_left, p_top + p_corners.topLeftSizeY      , -1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
				p_left, p_top + p_corners.topLeftSizeY      , 1.f , 0.f, 1.f, 0.f, 0.f, 0.f,
				p_left, p_bottom - p_corners.bottomLeftSizeY, 1.f , 0.f, 1.f, 0.f, 0.f, 0.f,
				p_left, p_bottom - p_corners.bottomLeftSizeY, 1.f , 0.f, 1.f, 0.f, 0.f, 0.f,
				p_left, p_bottom - p_corners.bottomLeftSizeY, -1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
				p_left, p_top + p_corners.topLeftSizeY      , -1.f, 0.f, 0.f, 0.f, 0.f, 0.f,

				p_right, p_top + p_corners.topRightSizeY      , -1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
				p_right, p_top + p_corners.topRightSizeY      , 1.f , 0.f, 1.f, 0.f, 0.f, 0.f,
				p_right, p_bottom - p_corners.bottomRightSizeY, 1.f , 0.f, 1.f, 0.f, 0.f, 0.f,
				p_right, p_bottom - p_corners.bottomRightSizeY, 1.f , 0.f, 1.f, 0.f, 0.f, 0.f,
				p_right, p_bottom - p_corners.bottomRightSizeY, -1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
				p_right, p_top + p_corners.topRightSizeY      , -1.f, 0.f, 0.f, 0.f, 0.f, 0.f
			}
		);

		if (p_corners.topLeftType == Avo::RectangleCornerType::Cut)
		{
			Avo::Point<> normal{-p_corners.topLeftSizeY, -p_corners.topLeftSizeX};
			normal.normalize();
			p_buffer.insert(
				p_buffer.end(),
				{
					p_left                         , p_top + p_corners.topLeftSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f,
					p_left                         , p_top + p_corners.topLeftSizeY, -1.f         , 0.f          , 1.f, 0.f, 0.f, 0.f,
					p_left + p_corners.topLeftSizeX, p_top                         , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
					p_left + p_corners.topLeftSizeX, p_top                         , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
					p_left + p_corners.topLeftSizeX, p_top                         , -normal.x*2.f, -normal.y    , 0.f, 0.f, 0.f, 0.f,
					p_left                         , p_top + p_corners.topLeftSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.topLeftSizeX*p_corners.topLeftSizeY);
			float lastX = p_corners.topLeftSizeX;
			float lastY = 0.f;
			Avo::Point<> lastNormal(-lastX, -lastY);
			lastNormal.normalizeFast();
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.topLeftSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.topLeftSizeY*s_unitCirclePoints[a].y;
				Avo::Point<> normal{-x, -y};
				normal.normalizeFast();
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - x    , p_top + p_corners.topLeftSizeY - y    , normal.x     , normal.y     , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - x    , p_top + p_corners.topLeftSizeY - y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - x    , p_top + p_corners.topLeftSizeY - y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
					}
				);
				lastX = x;
				lastY = y;
				lastNormal = normal;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x  , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX        , p_top                                 , 0.f           , -1.f         , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX        , p_top                                 , 0.f           , 1.f          , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX        , p_top                                 , 0.f           , 1.f          , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, -lastNormal.x , -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x  , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
			}
		}
		if (p_corners.topLeftType == Avo::RectangleCornerType::Cut)
		{
			Avo::Point<> normal{p_corners.topRightSizeY, -p_corners.topRightSizeX};
			normal.normalize();
			p_buffer.insert(
				p_buffer.end(),
				{
					p_right                          , p_top + p_corners.topRightSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f,
					p_right                          , p_top + p_corners.topRightSizeY, 1.f          , 0.f          , 1.f, 0.f, 0.f, 0.f,
					p_right - p_corners.topRightSizeX, p_top                          , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
					p_right - p_corners.topRightSizeX, p_top                          , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
					p_right - p_corners.topRightSizeX, p_top                          , -normal.x*2.f, -normal.y    , 0.f, 0.f, 0.f, 0.f,
					p_right                          , p_top + p_corners.topRightSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.topRightSizeX*p_corners.topRightSizeY);
			float lastX = p_corners.topRightSizeX;
			float lastY = 0.f;
			Avo::Point<> lastNormal{lastX, -lastY};
			lastNormal.normalizeFast();
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.topRightSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.topRightSizeY*s_unitCirclePoints[a].y;
				Avo::Point<> normal{x, -y};
				normal.normalizeFast();
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + x    , p_top + p_corners.topLeftSizeY - y    , normal.x     , normal.y     , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + x    , p_top + p_corners.topLeftSizeY - y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + x    , p_top + p_corners.topLeftSizeY - y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topLeftSizeY - lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
				lastNormal = normal;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX        , p_top                                 , 0.f          , -1.f         , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX        , p_top                                 , 0.f          , 1.f          , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX        , p_top                                 , 0.f          , 1.f          , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topLeftSizeY - lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topLeftSizeY - lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
			}
		}
		if (p_corners.topLeftType == Avo::RectangleCornerType::Cut)
		{
			Avo::Point<> normal{-p_corners.bottomLeftSizeY, p_corners.bottomLeftSizeX};
			normal.normalize();
			p_buffer.insert(
				p_buffer.end(),
				{
					p_left                            , p_bottom - p_corners.bottomLeftSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f,
					p_left                            , p_bottom - p_corners.bottomLeftSizeY, -1.f         , 0.f          , 1.f, 0.f, 0.f, 0.f,
					p_left + p_corners.bottomLeftSizeX, p_bottom                            , 0.f          , 1.f          , 1.f, 0.f, 0.f, 0.f,
					p_left + p_corners.bottomLeftSizeX, p_bottom                            , 0.f          , 1.f          , 1.f, 0.f, 0.f, 0.f,
					p_left + p_corners.bottomLeftSizeX, p_bottom                            , -normal.x*2.f, -normal.y    , 0.f, 0.f, 0.f, 0.f,
					p_left                            , p_bottom - p_corners.bottomLeftSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.bottomLeftSizeX*p_corners.bottomLeftSizeY);
			float lastX = p_corners.bottomLeftSizeX;
			float lastY = 0.f;
			Avo::Point<> lastNormal{-lastX, lastY};
			lastNormal.normalizeFast();
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.topLeftSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.topLeftSizeY*s_unitCirclePoints[a].y;
				Avo::Point<> normal{-x, y};
				normal.normalizeFast();
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - x    , p_bottom - p_corners.bottomLeftSizeY + y    , normal.x     , normal.y     , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - x    , p_bottom - p_corners.bottomLeftSizeY + y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - x    , p_bottom - p_corners.bottomLeftSizeY + y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
				lastNormal = normal;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX        , p_bottom                                    , 0.f          , 1.f          , 0.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX        , p_bottom                                    , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX        , p_bottom                                    , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
			}
		}
		if (p_corners.topLeftType == Avo::RectangleCornerType::Cut)
		{
			Avo::Point<> normal{p_corners.bottomLeftSizeY, p_corners.bottomLeftSizeX};
			normal.normalize();
			p_buffer.insert(
				p_buffer.end(),
				{
					p_right                             , p_bottom - p_corners.bottomLeftSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f,
					p_right                             , p_bottom - p_corners.bottomLeftSizeY, 1.f          , 0.f          , 1.f, 0.f, 0.f, 0.f,
					p_right - p_corners.bottomRightSizeX, p_bottom                            , 0.f          , 1.f          , 1.f, 0.f, 0.f, 0.f,
					p_right - p_corners.bottomRightSizeX, p_bottom                            , 0.f          , 1.f          , 1.f, 0.f, 0.f, 0.f,
					p_right - p_corners.bottomRightSizeX, p_bottom                            , -normal.x*2.f, -normal.y    , 0.f, 0.f, 0.f, 0.f,
					p_right                             , p_bottom - p_corners.bottomLeftSizeY, -normal.x    , -normal.y*2.f, 0.f, 0.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.bottomRightSizeX*p_corners.bottomRightSizeY);
			float lastX = p_corners.bottomRightSizeX;
			float lastY = 0.f;
			Avo::Point<> lastNormal{lastX, lastY};
			lastNormal.normalizeFast();
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.topRightSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.topRightSizeY*s_unitCirclePoints[a].y;
				Avo::Point<> normal{x, y};
				normal.normalizeFast();
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + x    , p_bottom - p_corners.bottomRightSizeY + y    , normal.x     , normal.y     , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + x    , p_bottom - p_corners.bottomRightSizeY + y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + x    , p_bottom - p_corners.bottomRightSizeY + y    , -normal.x    , -normal.y    , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
				lastNormal = normal;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX        , p_bottom                                     , 0.f          , 1.f          , 0.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX        , p_bottom                                     , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX        , p_bottom                                     , 0.f          , -1.f         , 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, -lastNormal.x, -lastNormal.y, 1.f, 0.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, lastNormal.x , lastNormal.y , 0.f, 0.f, 0.f, 0.f
					}
				);
			}
		}
	}
	/*
		The primitive type is GL_TRIANGLES
	*/
	static void createFilledCornerRectangleGeometry(std::vector<float>& p_buffer, float p_left, float p_top, float p_right, float p_bottom, Avo::RectangleCorners p_corners)
	{
		p_left -= 0.5f;
		p_top -= 0.5f;
		p_right += 0.5f;
		p_bottom += 0.5f;
		if (p_right - p_left < p_corners.topLeftSizeX + p_corners.topRightSizeY)
		{
			float factor = (p_right - p_left)/(p_corners.topLeftSizeX + p_corners.topRightSizeX);
			p_corners.topLeftSizeX *= factor;
			p_corners.topRightSizeX *= factor;
		}
		if (p_right - p_left < p_corners.bottomLeftSizeX + p_corners.bottomRightSizeY)
		{
			float factor = (p_right - p_left)/(p_corners.bottomLeftSizeX + p_corners.bottomRightSizeX);
			p_corners.bottomLeftSizeX *= factor;
			p_corners.bottomRightSizeX *= factor;
		}
		if (p_bottom - p_top < p_corners.topLeftSizeX + p_corners.bottomLeftSizeY)
		{
			float factor = (p_bottom - p_top)/(p_corners.topLeftSizeX + p_corners.bottomLeftSizeY);
			p_corners.topLeftSizeX *= factor;
			p_corners.bottomLeftSizeX *= factor;
		}
		if (p_bottom - p_top < p_corners.topRightSizeX + p_corners.bottomRightSizeY)
		{
			float factor = (p_bottom - p_top)/(p_corners.topRightSizeX + p_corners.bottomRightSizeY);
			p_corners.topRightSizeX *= factor;
			p_corners.bottomRightSizeX *= factor;
		}

		// Create the "body"
		float width = p_right - p_left;
		p_buffer.insert(
			p_buffer.end(),
			{
				// Horizontal center bar
				p_left , p_top + p_corners.topLeftSizeY       , 1.f, width, 0.5f, 2.f, 0.f, 0.f,
				p_right, p_top + p_corners.topRightSizeY      , 0.f, width, 0.5f, 2.f, 0.f, 0.f,
				p_right, p_bottom - p_corners.bottomRightSizeY, 0.f, width, 0.5f, 2.f, 0.f, 0.f,
				p_right, p_bottom - p_corners.bottomRightSizeY, 1.f, width, 0.5f, 2.f, 0.f, 0.f,
				p_left , p_bottom - p_corners.bottomLeftSizeY , 0.f, width, 0.5f, 2.f, 0.f, 0.f,
				p_left , p_top + p_corners.topLeftSizeY       , 0.f, width, 0.5f, 2.f, 0.f, 0.f,

				// Top center bar
				p_left + p_corners.topLeftSizeX  , p_top                          , 0.f , p_corners.topLeftSizeY*2.f , 0.5f, 2.f, 0.f, 0.f,
				p_right - p_corners.topRightSizeX, p_top                          , 0.f , p_corners.topRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
				p_right - p_corners.topRightSizeX, p_top + p_corners.topRightSizeY, 0.5f, p_corners.topRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
				p_right - p_corners.topRightSizeX, p_top + p_corners.topRightSizeY, 0.5f, p_corners.topRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
				p_left + p_corners.topLeftSizeX  , p_top + p_corners.topLeftSizeY , 0.5f, p_corners.topLeftSizeY*2.f , 0.5f, 2.f, 0.f, 0.f,
				p_left + p_corners.topLeftSizeX  , p_top                          , 0.f , p_corners.topLeftSizeY*2.f , 0.5f, 2.f, 0.f, 0.f,

				// Bottom center bar
				p_left + p_corners.bottomLeftSizeX  , p_bottom                             , 0.f , p_corners.bottomLeftSizeY*2.f , 0.5f, 2.f, 0.f, 0.f,
				p_right - p_corners.bottomRightSizeX, p_bottom                             , 0.f , p_corners.bottomRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
				p_right - p_corners.bottomRightSizeX, p_bottom - p_corners.bottomRightSizeY, 0.5f, p_corners.bottomRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
				p_right - p_corners.bottomRightSizeX, p_bottom - p_corners.bottomRightSizeY, 0.5f, p_corners.bottomRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
				p_left + p_corners.bottomLeftSizeX  , p_bottom - p_corners.bottomLeftSizeY , 0.5f, p_corners.bottomLeftSizeY*2.f , 0.5f, 2.f, 0.f, 0.f,
				p_left + p_corners.bottomLeftSizeX  , p_bottom                             , 0.f , p_corners.bottomLeftSizeY*2.f , 0.5f, 2.f, 0.f, 0.f
			}
		);

		// Create the corners
		if (p_corners.topLeftType == Avo::RectangleCornerType::Cut)
		{
			float distance = 2.f*p_corners.topLeftSizeX*p_corners.topLeftSizeY/(float)Avo::Point<>::getLengthFast(p_corners.topLeftSizeX, p_corners.topLeftSizeY);
			p_buffer.insert(
				p_buffer.end(),
				{
					p_left                         , p_top + p_corners.topLeftSizeY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_left + p_corners.topLeftSizeX, p_top                         , 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_left + p_corners.topLeftSizeX, p_top + p_corners.topLeftSizeY, 0.5f, distance, 0.5f, 2.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.topLeftSizeX*p_corners.topLeftSizeY);
			float lastX = p_corners.topLeftSizeX;
			float lastY = 0.f;
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.topLeftSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.topLeftSizeY*s_unitCirclePoints[a].y;
				// Length of vector (0.5*(x1 + x2), 0.5*(y1 + y2)), doubled because the anti-aliasing attribute of the inner vertex is 0.5 (we don't want anti-aliasing for that vertex)
				auto distance = (float)Avo::Point<>::getLengthFast(lastX + x, lastY + y);
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.topLeftSizeX        , p_top + p_corners.topLeftSizeY        , 0.5f, distance, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - x    , p_top + p_corners.topLeftSizeY - y    , 0.f , distance, 0.5f, 2.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.topLeftSizeX        , p_top + p_corners.topLeftSizeY        , 0.5f, p_corners.topLeftSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX - lastX, p_top + p_corners.topLeftSizeY - lastY, 0.f , p_corners.topLeftSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.topLeftSizeX        , p_top                                 , 0.f , p_corners.topLeftSizeY*2.f, 0.5f, 2.f, 0.f, 0.f
					}
				);
			}
		}
		if (p_corners.topRightType == Avo::RectangleCornerType::Cut)
		{
			float distance = 2.f*p_corners.topRightSizeX*p_corners.topRightSizeY/(float)Avo::Point<>::getLengthFast(p_corners.topRightSizeX, p_corners.topRightSizeY);
			p_buffer.insert(
				p_buffer.end(),
				{
					p_right                          , p_top + p_corners.topRightSizeY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_right - p_corners.topRightSizeX, p_top                          , 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_right - p_corners.topRightSizeX, p_top + p_corners.topRightSizeY, 0.5f, distance, 0.5f, 2.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.topRightSizeX*p_corners.topRightSizeY);
			float lastX = p_corners.topRightSizeX;
			float lastY = 0.f;
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.topRightSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.topRightSizeY*s_unitCirclePoints[a].y;
				auto distance = (float)Avo::Point<>::getLengthFast(lastX + x, lastY + y);
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.topRightSizeX        , p_top + p_corners.topRightSizeY        , 0.5f, distance, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topRightSizeY - lastY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + x    , p_top + p_corners.topRightSizeY - y    , 0.f , distance, 0.5f, 2.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.topRightSizeX        , p_top + p_corners.topRightSizeY        , 0.5f, p_corners.topRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX + lastX, p_top + p_corners.topRightSizeY - lastY, 0.f , p_corners.topRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.topRightSizeX        , p_top                                  , 0.f , p_corners.topRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f
					}
				);
			}
		}

		if (p_corners.bottomLeftType == Avo::RectangleCornerType::Cut)
		{
			float distance = 2.f*p_corners.bottomLeftSizeX*p_corners.bottomLeftSizeY/(float)Avo::Point<>::getLengthFast(p_corners.bottomLeftSizeX, p_corners.bottomLeftSizeY);
			p_buffer.insert(
				p_buffer.end(),
				{
					p_left                            , p_bottom - p_corners.bottomLeftSizeY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_left + p_corners.bottomLeftSizeX, p_bottom                            , 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_left + p_corners.bottomLeftSizeX, p_bottom - p_corners.bottomLeftSizeY, 0.5f, distance, 0.5f, 2.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.bottomLeftSizeX*p_corners.bottomLeftSizeY);
			float lastX = p_corners.bottomLeftSizeX;
			float lastY = 0.f;
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.bottomLeftSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.bottomLeftSizeY*s_unitCirclePoints[a].y;
				auto distance = (float)Avo::Point<>::getLengthFast(lastX + x, lastY + y);
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.bottomLeftSizeX        , p_bottom - p_corners.bottomLeftSizeY        , 0.5f, distance, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - x    , p_bottom - p_corners.bottomLeftSizeY + y    , 0.f , distance, 0.5f, 2.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_left + p_corners.bottomLeftSizeX        , p_bottom - p_corners.bottomLeftSizeY        , 0.5f, p_corners.bottomLeftSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX - lastX, p_bottom - p_corners.bottomLeftSizeY + lastY, 0.f , p_corners.bottomLeftSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_left + p_corners.bottomLeftSizeX        , p_bottom                                    , 0.f , p_corners.bottomLeftSizeY*2.f, 0.5f, 2.f, 0.f, 0.f
					}
				);
			}
		}

		if (p_corners.bottomRightType == Avo::RectangleCornerType::Cut)
		{
			float distance = 2.f*p_corners.bottomRightSizeX*p_corners.bottomRightSizeY/(float)Avo::Point<>::getLengthFast(p_corners.bottomRightSizeX, p_corners.bottomRightSizeY);
			p_buffer.insert(
				p_buffer.end(),
				{
					p_right                             , p_bottom - p_corners.bottomRightSizeY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_right - p_corners.bottomRightSizeX, p_bottom                             , 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
					p_right - p_corners.bottomRightSizeX, p_bottom - p_corners.bottomRightSizeY, 0.5f, distance, 0.5f, 2.f, 0.f, 0.f
				}
			);
		}
		else
		{
			uint32 stride = getCirclePointStrideForRadiusSquared(p_corners.bottomRightSizeX*p_corners.bottomRightSizeY);
			float lastX = p_corners.bottomRightSizeX;
			float lastY = 0.f;
			for (uint32 a = 1; a <= CIRCLE_RESOLUTION/4; a += stride)
			{
				float x = p_corners.bottomRightSizeX*s_unitCirclePoints[a].x;
				float y = p_corners.bottomRightSizeY*s_unitCirclePoints[a].y;
				auto distance = (float)Avo::Point<>::getLengthFast(lastX + x, lastY + y);
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.bottomRightSizeX        , p_bottom - p_corners.bottomRightSizeY        , 0.5f, distance, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, 0.f , distance, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + x    , p_bottom - p_corners.bottomRightSizeY + y    , 0.f , distance, 0.5f, 2.f, 0.f, 0.f
					}
				);
				lastX = x;
				lastY = y;
			}
			if (CIRCLE_RESOLUTION % (4*stride))
			{
				p_buffer.insert(
					p_buffer.end(),
					{
						p_right - p_corners.bottomRightSizeX        , p_bottom - p_corners.bottomRightSizeY        , 0.5f, p_corners.bottomRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX + lastX, p_bottom - p_corners.bottomRightSizeY + lastY, 0.f , p_corners.bottomRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f,
						p_right - p_corners.bottomRightSizeX        , p_bottom                                     , 0.f , p_corners.bottomRightSizeY*2.f, 0.5f, 2.f, 0.f, 0.f
					}
				);
			}
		}
	}

public:
	explicit OpenGlDrawingContext(Avo::Window* p_window) :
		m_window((LinuxWindow*)p_window), m_windowHandle((XWindow)p_window->getNativeHandle())
	{
		loadOpenGlFunctions();

		initializeStaticVariables();

		//------------------------------
		// Create the OpenGL context.
		// Need to find the framebuffer configuration that was used to create the window, because our context should match it.

		m_server = XOpenDisplay(nullptr);

		auto* linuxWindow = (LinuxWindow*)p_window;

		m_dipToPixelFactor = linuxWindow->getDipToPixelFactor();

		XWindowAttributes windowAttributes; // Used to match the VisualID
		XGetWindowAttributes(m_server, m_windowHandle, &windowAttributes);

		int numberOfConfigurations = 0;
		GLXFBConfig* framebufferConfigurations = glXChooseFBConfig(m_server, DefaultScreen(m_server), nullptr, &numberOfConfigurations);
		GLXFBConfig framebufferConfiguration = *framebufferConfigurations;
		for (uint32 a = 0; a < numberOfConfigurations; a++)
		{
			XVisualInfo* visualInfo = glXGetVisualFromFBConfig(m_server, framebufferConfigurations[a]);
			if (!visualInfo)
			{
				continue;
			}
			if (visualInfo->visualid == windowAttributes.visual->visualid)
			{
				framebufferConfiguration = framebufferConfigurations[a];
				XFree(visualInfo);
				break;
			}
			XFree(visualInfo);
		}

		int contextAttributes[] =
		{
			// Our minimum supported OpenGL version is 3.3
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 3,
			GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
			GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
			#ifdef _DEBUG
			| GLX_CONTEXT_DEBUG_BIT_ARB
			#endif
			, 0 // Null terminator
		};
		m_context = glXCreateContextAttribsARB(m_server, framebufferConfiguration, nullptr, true, contextAttributes);

		XFree(framebufferConfigurations);

		//------------------------------

		glXMakeCurrent(m_server, m_windowHandle, m_context);
		glViewport(0, 0, linuxWindow->getWidth()*m_dipToPixelFactor, linuxWindow->getHeight()*m_dipToPixelFactor);
		m_size = linuxWindow->getSize();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//------------------------------

		static char const* vertexShaderSource =
R"(
#version 330 core
layout (location = 0) in vec2 in_vertex;
layout (location = 1) in vec4 in_antiAliasAttribute;
layout (location = 2) in vec2 in_textureCoordinate;

out vec4 pass_antiAliasAttribute;
out vec2 pass_textureCoordinates;

uniform mat3 u_transform;
uniform mat3 u_viewTransform;
uniform int u_renderMode;
uniform float u_lineWidth;

void main()
{
	if (u_renderMode != 0)
	{
		pass_antiAliasAttribute = in_antiAliasAttribute;
		gl_Position = vec4(u_viewTransform*u_transform*vec3(in_vertex, 1.f), 1.f);
	}
	else
	{
		pass_antiAliasAttribute.xy = in_antiAliasAttribute.xy*(u_lineWidth*0.5f);
		gl_Position = vec4(u_viewTransform*u_transform*vec3(in_vertex + pass_antiAliasAttribute.xy, 1.f), 1.f);
	}
	if (u_renderMode == 2)
	{
		pass_textureCoordinates = in_textureCoordinate;
	}
}
)";

		// pass_edgeAttribute contains the data needed to perform anti-aliasing using automatic vertex attribute interpolation.
		static char const* fragmentShaderSource =
R"(
#version 330 core
in vec4 pass_antiAliasAttribute;
in vec2 pass_textureCoordinates;

out vec4 out_fragmentColor;

uniform vec4 u_color;
uniform int u_renderMode;
uniform float u_lineWidth;
uniform sampler2D u_texture;

void main()
{
	if (u_renderMode == 0) {
		out_fragmentColor = vec4(u_color.rgb, u_color.a*min(pass_antiAliasAttribute.z*u_lineWidth, (1.f - pass_antiAliasAttribute.z)*u_lineWidth));
	}
	else if (u_renderMode == 1) {
		out_fragmentColor = vec4(u_color.rgb, u_color.a*min(min(pass_antiAliasAttribute.z*pass_antiAliasAttribute.w, (1.f - pass_antiAliasAttribute.z)*pass_antiAliasAttribute.w), min(pass_antiAliasAttribute.x*pass_antiAliasAttribute.y, (1.f - pass_antiAliasAttribute.x)*pass_antiAliasAttribute.y)));
	}
	else if (u_renderMode == 2) {
		out_fragmentColor = texture(u_texture, pass_textureCoordinates);
		out_fragmentColor.a *= min(min(pass_antiAliasAttribute.z*pass_antiAliasAttribute.w, (1.f - pass_antiAliasAttribute.z)*pass_antiAliasAttribute.w), min(pass_antiAliasAttribute.x*pass_antiAliasAttribute.y, (1.f - pass_antiAliasAttribute.x)*pass_antiAliasAttribute.y));
	}
}
)";
		m_renderShader.compile(vertexShaderSource, fragmentShaderSource);
		m_renderShader.use();

		// Identity matrix
		m_transformMatrix[0] = 1.f;
		m_transformMatrix[4] = 1.f;
		m_transformMatrix[8] = 1.f;

		m_renderShader.setUniformMatrix3x3("u_transform", m_transformMatrix);

		//------------------------------

		m_drawCalls.reserve(256);
		m_vertexBuffer.reserve(8192);
	}
	~OpenGlDrawingContext() final
	{
		glXDestroyContext(m_server, m_context);
		XCloseDisplay(m_server);
	}

	//------------------------------

	void beginDrawing() override
	{
		glXMakeCurrent(m_server, m_windowHandle, m_context);
		m_drawCalls.clear();
		m_vertexBuffer.clear();

		//glClear(GL_COLOR_BUFFER_BIT);
	}
	void finishDrawing(std::vector<Avo::Rectangle<>> const& p_updatedRectangles) override
	{
		// GL_STATIC_DRAW hints that the buffer is modified once and used many times
		m_renderShader.setVertexData(m_vertexBuffer.data(), m_vertexBuffer.size()*sizeof(float), VERTEX_DATA_LENGTH*sizeof(float), GL_STATIC_DRAW);
		m_renderShader.configureVertexShaderInput(0, 2, 0);
		m_renderShader.configureVertexShaderInput(1, 4, 2);
		m_renderShader.configureVertexShaderInput(2, 2, 6);

		for (auto& drawCall : m_drawCalls)
		{
			m_renderShader.setUniformColor("u_color", drawCall.color);
			m_renderShader.setUniformMatrix3x3("u_transform", drawCall.transform);
			if (drawCall.renderMode == 0)
			{
				m_renderShader.setUniformValue("u_lineWidth", drawCall.lineWidth);
			}
			else if (drawCall.renderMode == 2)
			{
				glBindTexture(GL_TEXTURE_2D, drawCall.texture);
			}
			m_renderShader.setUniformValue("u_renderMode", drawCall.renderMode);
			m_renderShader.draw(drawCall.vertexBufferStartIndex, drawCall.numberOfVertices, drawCall.primitiveType);
		}

		//------------------------------

		glXSwapBuffers(m_server, m_windowHandle);
	}

	//------------------------------

	Avo::DrawingState* createDrawingState() override
	{
		return nullptr;
	}

	void saveDrawingState(Avo::DrawingState* p_drawingState) override
	{
	}

	void restoreDrawingState(Avo::DrawingState* p_drawingState) override
	{
	}

	//------------------------------

	void setIsFullscreen(bool p_isFullscreen) override
	{
	}
	void switchFullscreen() override
	{
	}
	bool getIsFullscreen() override
	{
	}

	//------------------------------

	void enableVsync() override
	{
	}
	void disableVsync() override
	{
	}
	bool getIsVsyncEnabled() override
	{
		return true;
	}

	//------------------------------

	void setBackgroundColor(Avo::Color p_color) override
	{
		// If we try to set the background color here directly, some methods block until XNextEvent in the event thread returns...
		// So send the color in a client event instead and let the event thread set the background color of the window.

		//Display* server = m_window->getServer();
		//XLockDisplay(server);
		//
		//XEvent event;
		//event.type = ClientMessage;
		//event.xclient.message_type = m_window->getBackgroundColorMessageType();
		//event.xclient.format = 32; // Using data.l, 32 bits per data item
		//event.xclient.data.l[0] = long(p_color.red*65535);
		//event.xclient.data.l[1] = long(p_color.green*65535);
		//event.xclient.data.l[2] = long(p_color.blue*65535);
		//event.xclient.display = server;
		//event.xclient.window = m_windowHandle;
		//event.xclient.send_event = true;
		//XSendEvent(server, m_windowHandle, false, 0, &event);
		//XFlush(server);
		//
		//XUnlockDisplay(server);

		//XColor xColor;
		//xColor.red = (unsigned short)(p_color.red*65535);
		//xColor.green = (unsigned short)(p_color.green*65535);
		//xColor.blue = (unsigned short)(p_color.blue*65535);
		//xColor.flags = DoRed | DoGreen | DoBlue;
		//XAllocColor(m_server, m_window->getColormap(), &xColor);
		//XSetWindowBackground(m_server, m_windowHandle, xColor.pixel);

		m_backgroundColor = p_color;
	}
	Avo::Color getBackgroundColor() override
	{
		return m_backgroundColor;
	}

	//------------------------------

	float getDpi() override
	{
		return m_dipToPixelFactor*NORMAL_DPI;
	}
	void setDpi(float p_dpi) override
	{
		m_dipToPixelFactor = p_dpi/NORMAL_DPI;
	}

	//------------------------------

	void moveOrigin(Avo::Point<> p_offset) override
	{
		moveOrigin(p_offset.x, p_offset.y);
	}
	void moveOrigin(float p_offsetX, float p_offsetY) override
	{
		m_transformMatrix[7] -= p_offsetX;
		m_transformMatrix[8] -= p_offsetY;
	}
	void setOrigin(Avo::Point<> p_origin) override
	{
		setOrigin(p_origin.x, p_origin.y);
	}
	void setOrigin(float p_x, float p_y) override
	{
		m_transformMatrix[7] = -p_x;
		m_transformMatrix[8] = -p_y;
	}
	Avo::Point<> getOrigin() override
	{
		return {-m_transformMatrix[7], -m_transformMatrix[8]};
	}

	//------------------------------

	void scale(float p_scale) override
	{
		scale(p_scale, p_scale);
	}
	void scale(float p_scaleX, float p_scaleY) override
	{
		m_scale.x *= p_scaleX;
		m_scale.y *= p_scaleY;
		m_transformMatrix[0] *= p_scaleX;
		m_transformMatrix[1] *= p_scaleY;
		m_transformMatrix[3] *= p_scaleX;
		m_transformMatrix[4] *= p_scaleY;
	}
	void scale(float p_scale, Avo::Point<> p_origin) override
	{
		scale(p_scale, p_scale, p_origin.x, p_origin.y);
	}
	void scale(float p_scaleX, float p_scaleY, Avo::Point<> p_origin) override
	{
		scale(p_scaleX, p_scaleY, p_origin.x, p_origin.y);
	}
	void scale(float p_scale, float p_originX, float p_originY) override
	{
		scale(p_scale, p_scale, p_originX, p_originY);
	}
	void scale(float p_scaleX, float p_scaleY, float p_originX, float p_originY) override
	{
		m_scale.x *= p_scaleX;
		m_scale.y *= p_scaleY;
		m_transformMatrix[0] *= p_scaleX;
		m_transformMatrix[1] *= p_scaleX;
		m_transformMatrix[3] *= p_scaleY;
		m_transformMatrix[4] *= p_scaleY;
		m_transformMatrix[6] += (p_originX - m_transformMatrix[6])*(1.f - p_scaleX);
		m_transformMatrix[7] += (p_originY - m_transformMatrix[7])*(1.f - p_scaleY);
	}
	void setScale(float p_scale) override
	{
		scale(p_scale/m_scale.x, p_scale/m_scale.y);
	}
	void setScale(float p_scaleX, float p_scaleY) override
	{
		scale(p_scaleX/m_scale.x, p_scaleY/m_scale.y);
	}
	void setScale(float p_scale, Avo::Point<> p_origin) override
	{
		scale(p_scale/m_scale.x, p_scale/m_scale.y, p_origin.x, p_origin.y);
	}
	void setScale(float p_scaleX, float p_scaleY, Avo::Point<> p_origin) override
	{
		scale(p_scaleX/m_scale.x, p_scaleY/m_scale.y, p_origin.x, p_origin.y);
	}
	void setScale(float p_scale, float p_originX, float p_originY) override
	{
		scale(p_scale/m_scale.x, p_scale/m_scale.y, p_originX, p_originY);
	}
	void setScale(float p_scaleX, float p_scaleY, float p_originX, float p_originY) override
	{
		scale(p_scaleX/m_scale.x, p_scaleY/m_scale.y, p_originX, p_originY);
	}
	Avo::Point<> getScale() override
	{
		return m_scale;
	}
	float getScaleX() override
	{
		return m_scale.x;
	}
	float getScaleY() override
	{
		return m_scale.y;
	}

	//------------------------------

	void rotate(float p_radians) override
	{
		float cos = std::cos(p_radians);
		float sin = std::sin(p_radians);

		float previousMatrix[9];
		memcpy(previousMatrix, m_transformMatrix, sizeof(previousMatrix));

		/*
			Matrix multiplication:
			[cos -sin  0]   [m[0] m[3] m[6]]   [m[0]cos - m[1]sin  m[3]cos - m[4]sin  m[6]cos - m[7]sin]
			[sin  cos  0]*[m[1] m[4] m[7]] = [m[0]sin + m[1]cos  m[3]sin + m[4]cos  m[6]sin + m[7]cos]
			[0    0    1]   [0    0    1   ]   [0                  0                  1                ]
		*/
		m_transformMatrix[0] = cos*previousMatrix[0] - sin*previousMatrix[1];
		m_transformMatrix[1] = sin*previousMatrix[0] + cos*previousMatrix[1];
		m_transformMatrix[3] = cos*previousMatrix[3] - sin*previousMatrix[4];
		m_transformMatrix[4] = sin*previousMatrix[3] + cos*previousMatrix[4];
		m_transformMatrix[6] = cos*previousMatrix[6] - sin*previousMatrix[7];
		m_transformMatrix[7] = sin*previousMatrix[6] + cos*previousMatrix[7];
	}
	void rotate(float p_radians, Avo::Point<> p_origin) override
	{
		rotate(p_radians, p_origin.x, p_origin.y);
	}
	void rotate(float p_radians, float p_originX, float p_originY) override
	{
		float cos = std::cos(p_radians);
		float sin = std::sin(p_radians);

		float previousMatrix[9];
		memcpy(previousMatrix, m_transformMatrix, sizeof(previousMatrix));
		previousMatrix[6] -= p_originX;
		previousMatrix[7] -= p_originY;

		/*
			Matrix multiplication:
			[cos -sin  0]   [m[0] m[3] m[6]]   [m[0]cos - m[1]sin  m[3]cos - m[4]sin  m[6]cos - m[7]sin]
			[sin  cos  0]*[m[1] m[4] m[7]] = [m[0]sin + m[1]cos  m[3]sin + m[4]cos  m[6]sin + m[7]cos]
			[0    0    1]   [0    0    1   ]   [0                  0                  1                ]
		*/
		m_transformMatrix[0] = cos*previousMatrix[0] - sin*previousMatrix[1];
		m_transformMatrix[1] = sin*previousMatrix[0] + cos*previousMatrix[1];
		m_transformMatrix[3] = cos*previousMatrix[3] - sin*previousMatrix[4];
		m_transformMatrix[4] = sin*previousMatrix[3] + cos*previousMatrix[4];
		m_transformMatrix[6] = cos*previousMatrix[6] - sin*previousMatrix[7] + p_originX;
		m_transformMatrix[7] = sin*previousMatrix[6] + cos*previousMatrix[7] + p_originY;
	}

	//------------------------------

	void resetTransformations() override
	{
		m_transformMatrix[0] = 1.f;
		m_transformMatrix[1] = 0.f;
		m_transformMatrix[2] = 0.f;
		m_transformMatrix[3] = 0.f;
		m_transformMatrix[4] = 1.f;
		m_transformMatrix[5] = 0.f;
		m_transformMatrix[6] = 0.f;
		m_transformMatrix[7] = 0.f;
		m_transformMatrix[8] = 1.f;
	}

	//------------------------------

	void setSize(Avo::Point<> p_size) override
	{
		setSize(p_size.x, p_size.y);
	}
	void setSize(float p_width, float p_height) override
	{
		glXMakeCurrent(m_server, m_windowHandle, m_context);
		glViewport(0, 0, p_width*m_dipToPixelFactor, p_height*m_dipToPixelFactor);
		m_size.set(p_width, p_height);

		// Column-major order matrix
		float transform[9] =
		{
			2.f/m_size.x, 0.f          , 0.f,
			0.f         , -2.f/m_size.y, 0.f,
			-1.f        , 1.f          , 1.f
		};
		m_renderShader.setUniformMatrix3x3("u_viewTransform", transform);
	}
	Avo::Point<> getSize() override
	{
		return m_size;
	}

	//------------------------------

	void clear(Avo::Color p_color) override
	{
		glClearColor(p_color.red, p_color.green, p_color.blue, p_color.alpha);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	void clear() override
	{
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	//------------------------------

	void fillRectangle(float p_left, float p_top, float p_right, float p_bottom) override
	{
		// Anti-aliasing requires an extra row of pixels
		p_left -= 0.5f;
		p_top -= 0.5f;
		p_right += 0.5f;
		p_bottom += 0.5f;

		float height = p_bottom - p_top;
		float width = p_right - p_left;

		m_vertexBuffer.insert(
			m_vertexBuffer.end(),
			{
				p_left,  p_top,    1.f, height, 0.f, width, 0.f, 0.f,
				p_left,  p_bottom, 0.f, height, 0.f, width, 0.f, 0.f,
				p_right, p_bottom, 0.f, height, 1.f, width, 0.f, 0.f,

				p_right, p_bottom, 1.f, height, 0.f, width, 0.f, 0.f,
				p_left,  p_top,    0.f, height, 1.f, width, 0.f, 0.f,
				p_right, p_top,    0.f, height, 0.f, width, 0.f, 0.f
			}
		);
		addDrawCall(6u);
	}
	void fillRectangle(Avo::Point<> p_position, Avo::Point<> p_size) override
	{
		fillRectangle(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y);
	}
	void fillRectangle(Avo::Rectangle<> p_rectangle) override
	{
		fillRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom);
	}
	void fillRectangle(float p_width, float p_height) override
	{
		fillRectangle(0.f, 0.f, p_width, p_height);
	}
	void fillRectangle(Avo::Point<> p_size) override
	{
		fillRectangle(0.f, 0.f, p_size.x, p_size.y);
	}

	void fillRectangle(Avo::Rectangle<> p_rectangle, Avo::RectangleCorners const& p_rectangleCorners) override
	{
		fillRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, p_rectangleCorners);
	}
	void fillRectangle(Avo::Point<> p_position, Avo::Point<> p_size, Avo::RectangleCorners const& p_rectangleCorners) override
	{
		fillRectangle(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, p_rectangleCorners);
	}
	void fillRectangle(Avo::Point<> p_size, Avo::RectangleCorners const& p_rectangleCorners) override
	{
		fillRectangle(0.f, 0.f, p_size.x, p_size.y, p_rectangleCorners);
	}
	void fillRectangle(float p_width, float p_height, Avo::RectangleCorners const& p_rectangleCorners) override
	{
		fillRectangle(0.f, 0.f, p_width, p_height, p_rectangleCorners);
	}
	void fillRectangle(float p_left, float p_top, float p_right, float p_bottom, Avo::RectangleCorners const& p_rectangleCorners) override
	{
		size_t sizeBefore = m_vertexBuffer.size();
		createFilledCornerRectangleGeometry(m_vertexBuffer, p_left, p_top, p_right, p_bottom, p_rectangleCorners);
		addDrawCall((m_vertexBuffer.size() - sizeBefore)/VERTEX_DATA_LENGTH);
	}

	void fillRoundedRectangle(Avo::Rectangle<> p_rectangle, float p_radius) override
	{
		fillRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, Avo::RectangleCorners(p_radius));
	}
	void fillRoundedRectangle(Avo::Point<> p_position, Avo::Point<> p_size, float p_radius) override
	{
		fillRectangle(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, Avo::RectangleCorners(p_radius));
	}
	void fillRoundedRectangle(Avo::Point<> p_size, float p_radius) override
	{
		fillRectangle(0.f, 0.f, p_size.x, p_size.y, Avo::RectangleCorners(p_radius));
	}
	void fillRoundedRectangle(float p_width, float p_height, float p_radius) override
	{
		fillRectangle(0.f, 0.f, p_width, p_height, Avo::RectangleCorners(p_radius));
	}
	void fillRoundedRectangle(float p_left, float p_top, float p_right, float p_bottom, float p_radius) override
	{
		fillRectangle(p_left, p_top, p_right, p_bottom, Avo::RectangleCorners(p_radius));
	}

	//------------------------------

	void strokeRectangle(Avo::Rectangle<> p_rectangle, float p_strokeWidth) override
	{
		strokeRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, p_strokeWidth);
	}
	void strokeRectangle(Avo::Point<> p_position, Avo::Point<> p_size, float p_strokeWidth) override
	{
		strokeRectangle(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, p_strokeWidth);
	}
	void strokeRectangle(Avo::Point<> p_size, float p_strokeWidth) override
	{
		strokeRectangle(0.f, 0.f, p_size.x, p_size.y, p_strokeWidth);
	}
	void strokeRectangle(float p_width, float p_height, float p_strokeWidth) override
	{
		strokeRectangle(0.f, 0.f, p_width, p_height, p_strokeWidth);
	}
	void strokeRectangle(float p_left, float p_top, float p_right, float p_bottom, float p_strokeWidth) override
	{
		// Anti-aliasing requires an extra row of pixels
		p_strokeWidth += 1.f;
		p_left -= p_strokeWidth*0.5f;
		p_top -= p_strokeWidth*0.5f;
		p_right += p_strokeWidth*0.5f;
		p_bottom += p_strokeWidth*0.5f;

		float height = p_bottom - p_top;
		float width = p_right - p_left;
		m_vertexBuffer.insert(
			m_vertexBuffer.end(),
			{
				// Top
				p_left , p_top                , 1.f, p_strokeWidth, 0.f, width, 0.f, 0.f,
				p_left , p_top + p_strokeWidth, 0.f, p_strokeWidth, 0.f, width, 0.f, 0.f,
				p_right, p_top + p_strokeWidth, 0.f, p_strokeWidth, 1.f, width, 0.f, 0.f,
				p_right, p_top + p_strokeWidth, 1.f, p_strokeWidth, 0.f, width, 0.f, 0.f,
				p_left , p_top                , 0.f, p_strokeWidth, 1.f, width, 0.f, 0.f,
				p_right, p_top                , 0.f, p_strokeWidth, 0.f, width, 0.f, 0.f,

				// Bottom
				p_left , p_bottom - p_strokeWidth, 1.f, p_strokeWidth, 0.f, width, 0.f, 0.f,
				p_left , p_bottom                , 0.f, p_strokeWidth, 0.f, width, 0.f, 0.f,
				p_right, p_bottom                , 0.f, p_strokeWidth, 1.f, width, 0.f, 0.f,
				p_right, p_bottom                , 1.f, p_strokeWidth, 0.f, width, 0.f, 0.f,
				p_left , p_bottom - p_strokeWidth, 0.f, p_strokeWidth, 1.f, width, 0.f, 0.f,
				p_right, p_bottom - p_strokeWidth, 0.f, p_strokeWidth, 0.f, width, 0.f, 0.f,

				// Left
				p_left                , p_top + p_strokeWidth - 1.f   , 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_left                , p_bottom - p_strokeWidth + 1.f, 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_left + p_strokeWidth, p_bottom - p_strokeWidth + 1.f, 0.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_left + p_strokeWidth, p_bottom - p_strokeWidth + 1.f, 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_left                , p_top + p_strokeWidth - 1.f   , 0.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_left + p_strokeWidth, p_top + p_strokeWidth - 1.f   , 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,

				// Right
				p_right - p_strokeWidth, p_top + p_strokeWidth - 1.f   , 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_right - p_strokeWidth, p_bottom - p_strokeWidth + 1.f, 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_right                , p_bottom - p_strokeWidth + 1.f, 0.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_right                , p_bottom - p_strokeWidth + 1.f, 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_right - p_strokeWidth, p_top + p_strokeWidth - 1.f   , 0.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
				p_right                , p_top + p_strokeWidth - 1.f   , 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f,
			}
		);
		addDrawCall(24);
	}

	void strokeRectangle(Avo::Rectangle<> p_rectangle, Avo::RectangleCorners const& p_rectangleCorners, float p_strokeWidth) override
	{
		strokeRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, p_rectangleCorners, p_strokeWidth);
	}
	void strokeRectangle(Avo::Point<> p_position, Avo::Point<> p_size, Avo::RectangleCorners const& p_rectangleCorners, float p_strokeWidth) override
	{
		strokeRectangle(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, p_rectangleCorners, p_strokeWidth);
	}
	void strokeRectangle(Avo::Point<> p_size, Avo::RectangleCorners const& p_rectangleCorners, float p_strokeWidth) override
	{
		strokeRectangle(0.f, 0.f, p_size.x, p_size.y, p_rectangleCorners, p_strokeWidth);
	}
	void strokeRectangle(float p_width, float p_height, Avo::RectangleCorners const& p_rectangleCorners, float p_strokeWidth) override
	{
		strokeRectangle(0.f, 0.f, p_width, p_height, p_rectangleCorners, p_strokeWidth);
	}
	void strokeRectangle(float p_left, float p_top, float p_right, float p_bottom, Avo::RectangleCorners const& p_rectangleCorners, float p_strokeWidth) override
	{
		p_strokeWidth += 1.f;
		size_t sizeBefore = m_vertexBuffer.size();
		createStrokedCornerRectangleGeometry(m_vertexBuffer, p_left, p_top, p_right, p_bottom, p_rectangleCorners);
		addDrawCall((m_vertexBuffer.size() - sizeBefore)/VERTEX_DATA_LENGTH, p_strokeWidth);
	}

	void strokeRoundedRectangle(Avo::Rectangle<> p_rectangle, float p_radius, float p_strokeWidth) override
	{
		strokeRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, Avo::RectangleCorners(p_radius), p_strokeWidth);
	}
	void strokeRoundedRectangle(Avo::Point<> p_position, Avo::Point<> p_size, float p_radius, float p_strokeWidth) override
	{
		strokeRectangle(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, Avo::RectangleCorners(p_radius), p_strokeWidth);
	}
	void strokeRoundedRectangle(float p_left, float p_top, float p_right, float p_bottom, float p_radius, float p_strokeWidth) override
	{
		strokeRectangle(p_left, p_top, p_right, p_bottom, Avo::RectangleCorners(p_radius), p_strokeWidth);
	}

	void strokeRoundedRectangle(Avo::Point<> p_size, float p_radius, float p_strokeWidth) override
	{
		strokeRectangle(0.f, 0.f, p_size.x, p_size.y, Avo::RectangleCorners(p_radius), p_strokeWidth);
	}
	void strokeRoundedRectangle(float p_width, float p_height, float p_radius, float p_strokeWidth) override
	{
		strokeRectangle(0.f, 0.f, p_width, p_height, Avo::RectangleCorners(p_radius), p_strokeWidth);
	}

	//------------------------------

	void fillCircle(Avo::Point<> p_position, float p_radius) override
	{
		fillCircle(p_position.x, p_position.y, p_radius);
	}
	void fillCircle(float p_x, float p_y, float p_radius) override
	{
		p_radius += 0.5f;
		m_vertexBuffer.insert(m_vertexBuffer.end(), {p_x, p_y, 0.5f, p_radius*2.f, 0.5f, 2.f, 0.f, 0.f}); // Only want anti-aliasing on the outer edges of the triangles.
		for (uint32 a = 0; a <= CIRCLE_RESOLUTION; a++)
		{
			m_vertexBuffer.insert(m_vertexBuffer.end(), {s_unitCirclePoints[a].x*p_radius + p_x, s_unitCirclePoints[a].y*p_radius + p_y, 0.f, p_radius*2.f, 0.5f, 2.f, 0.f, 0.f,});
		}
		addDrawCall(CIRCLE_RESOLUTION + 2, 0u, (GLenum)GL_TRIANGLE_FAN);
	}

	void strokeCircle(Avo::Point<> p_position, float p_radius, float p_strokeWidth) override
	{
		strokeCircle(p_position.x, p_position.y, p_radius, p_strokeWidth);
	}
	void strokeCircle(float p_x, float p_y, float p_radius, float p_strokeWidth) override
	{
		p_strokeWidth += 1.f; // Anti-aliasing needs extra pixels
		p_radius += p_strokeWidth*0.5f;

		float innerAntiAliasingStart = 1.f - p_radius/p_strokeWidth;
		m_vertexBuffer.insert(m_vertexBuffer.end(), {p_x, p_y, innerAntiAliasingStart, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f});
		for (uint32 a = 0; a <= CIRCLE_RESOLUTION; a++)
		{
			m_vertexBuffer.insert(m_vertexBuffer.end(), {s_unitCirclePoints[a].x*p_radius + p_x, s_unitCirclePoints[a].y*p_radius + p_y, 1.f, p_strokeWidth, 0.5f, 2.f, 0.f, 0.f});
		}
		addDrawCall(CIRCLE_RESOLUTION + 2, 0u, (GLenum)GL_TRIANGLE_FAN);
	}

	//------------------------------

	void drawLine(Avo::Point<> p_point_0, Avo::Point<> p_point_1, float p_thickness) override
	{
		drawLine(p_point_0.x, p_point_0.y, p_point_1.x, p_point_1.y);
	}
	void drawLine(float p_x0, float p_y0, float p_x1, float p_y1, float p_thickness = 1.f) override
	{
		float distance = Avo::fastSqrt((p_x1 - p_x0)*(p_x1 - p_x0) + (p_y1 - p_y0)*(p_y1 - p_y0));
		float normalX = (p_y1 - p_y0)/distance;
		float normalY = -(p_x1 - p_x0)/distance;

		// Anti-aliasing requires extra pixels
		p_x0 += 0.5f*normalY;
		p_y0 -= 0.5f*normalX;
		p_x1 -= 0.5f*normalY;
		p_y1 += 0.5f*normalX;
		distance += 1.f;

		p_thickness += 1.f;
		normalX *= p_thickness*0.5f;
		normalY *= p_thickness*0.5f;

		m_vertexBuffer.insert(
			m_vertexBuffer.end(),
			{
				p_x0 + normalX, p_y0 + normalY, 1.f, p_thickness, 0.f, distance, 0.f, 0.f,
				p_x0 - normalX, p_y0 - normalY, 0.f, p_thickness, 0.f, distance, 0.f, 0.f,
				p_x1 - normalX, p_y1 - normalY, 0.f, p_thickness, 1.f, distance, 0.f, 0.f,

				p_x1 - normalX, p_y1 - normalY, 1.f, p_thickness, 0.f, distance, 0.f, 0.f,
				p_x0 + normalX, p_y0 + normalY, 0.f, p_thickness, 1.f, distance, 0.f, 0.f,
				p_x1 + normalX, p_y1 + normalY, 0.f, p_thickness, 0.f, distance, 0.f, 0.f
			}
		);
		addDrawCall(6u);
	}

	//------------------------------

	void strokeShape(std::vector<Avo::Point<>> const& p_vertices, float p_lineThickness, bool p_isClosed) override
	{
	}
	void strokeShape(Avo::Point<> const* p_vertices, uint32 p_numberOfVertices, float p_lineThickness, bool p_isClosed) override
	{
	}
	void fillShape(std::vector<Avo::Point<>> const& p_vertices) override
	{
	}
	void fillShape(Avo::Point<> const* p_vertices, uint32 p_numberOfVertices) override
	{
	}

	//------------------------------

	void strokeGeometry(Avo::Geometry const& p_geometry, float p_strokeWidth) override
	{
		auto geometry = (OpenGlGeometry*)getGeometryImplementation(p_geometry);
		m_vertexBuffer.insert(m_vertexBuffer.end(), geometry->vertexBuffer.begin(), geometry->vertexBuffer.end());
		addDrawCall(geometry->vertexBuffer.size(), p_strokeWidth);
	}
	void fillGeometry(Avo::Geometry const& p_geometry) override
	{
		auto geometry = (OpenGlGeometry*)getGeometryImplementation(p_geometry);
		m_vertexBuffer.insert(m_vertexBuffer.end(), geometry->vertexBuffer.begin(), geometry->vertexBuffer.end());
		addDrawCall(geometry->vertexBuffer.size());
	}

	//------------------------------

	Avo::Geometry createRoundedRectangleGeometry(float p_left, float p_top, float p_right, float p_bottom, float p_radius, bool p_isStroked) override
	{
		createCornerRectangleGeometry(p_left, p_top, p_right, p_bottom, Avo::RectangleCorners(p_radius), p_isStroked);
	}
	Avo::Geometry createRoundedRectangleGeometry(Avo::Point<> p_position, Avo::Point<> p_size, float p_radius, bool p_isStroked) override
	{
		createCornerRectangleGeometry(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, Avo::RectangleCorners{p_radius}, p_isStroked);
	}
	Avo::Geometry createRoundedRectangleGeometry(Avo::Rectangle<> p_rectangle, float p_radius, bool p_isStroked) override
	{
		createCornerRectangleGeometry(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, Avo::RectangleCorners{p_radius}, p_isStroked);
	}
	Avo::Geometry createRoundedRectangleGeometry(float p_width, float p_height, float p_radius, bool p_isStroked) override
	{
		createCornerRectangleGeometry(0.f, 0.f, p_width, p_height, Avo::RectangleCorners{p_radius}, p_isStroked);
	}
	Avo::Geometry createRoundedRectangleGeometry(Avo::Point<> p_size, float p_radius, bool p_isStroked) override
	{
		createCornerRectangleGeometry(0.f, 0.f, p_size.x, p_size.y, Avo::RectangleCorners{p_radius}, p_isStroked);
	}

	Avo::Geometry createCornerRectangleGeometry(float p_left, float p_top, float p_right, float p_bottom, Avo::RectangleCorners const& p_corners, bool p_isStroked) override
	{
		auto geometry = new OpenGlGeometry;

		if (p_isStroked)
		{
			createStrokedCornerRectangleGeometry(geometry->vertexBuffer, p_left, p_top, p_right, p_bottom, p_corners);
		}
		else
		{
			createFilledCornerRectangleGeometry(geometry->vertexBuffer, p_left, p_top, p_right, p_bottom, p_corners);
		}
		geometry->isStroked = p_isStroked;

		return createGeometryFromImplementation(geometry);
	}
	Avo::Geometry createCornerRectangleGeometry(Avo::Point<> p_position, Avo::Point<> p_size, Avo::RectangleCorners const& p_corners, bool p_isStroked) override
	{
		return createCornerRectangleGeometry(p_position.x, p_position.y, p_position.x + p_size.x, p_position.y + p_size.y, p_corners, p_isStroked);
	}
	Avo::Geometry createCornerRectangleGeometry(Avo::Rectangle<> p_rectangle, Avo::RectangleCorners const& p_corners, bool p_isStroked) override
	{
		return createCornerRectangleGeometry(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, p_corners, p_isStroked);
	}
	Avo::Geometry createCornerRectangleGeometry(float p_width, float p_height, Avo::RectangleCorners const& p_corners, bool p_isStroked) override
	{
		return createCornerRectangleGeometry(0.f, 0.f, p_width, p_height, p_corners, p_isStroked);
	}
	Avo::Geometry createCornerRectangleGeometry(Avo::Point<> p_size, Avo::RectangleCorners const& p_corners, bool p_isStroked) override
	{
		return createCornerRectangleGeometry(0.f, 0.f, p_size.x, p_size.y, p_corners, p_isStroked);
	}

	//------------------------------

	Avo::Geometry createPolygonGeometry(Avo::Point<> const* p_vertices, uint32 p_numberOfVertices, bool p_isStroked, bool p_isClosed) override
	{
		return createGeometryFromImplementation(new OpenGlGeometry);
	}

	//------------------------------

	void setLineCap(Avo::LineCap p_lineCap) override
	{
	}
	void setStartLineCap(Avo::LineCap p_lineCap) override
	{
	}
	void setEndLineCap(Avo::LineCap p_lineCap) override
	{
	}
	Avo::LineCap getStartLineCap() override
	{
	}
	Avo::LineCap getEndLineCap() override
	{
	}

	//------------------------------

	void setLineDashStyle(Avo::LineDashStyle p_dashStyle) override
	{
	}
	Avo::LineDashStyle getLineDashStyle() override
	{
	}

	void setLineDashOffset(float p_dashOffset) override
	{
	}
	float getLineDashOffset() override
	{
	}

	void setLineDashCap(Avo::LineCap p_dashCap) override
	{
	}
	Avo::LineCap getLineDashCap() override
	{
	}

	//------------------------------

	void setLineJoin(Avo::LineJoin p_lineJoin) override
	{
	}
	Avo::LineJoin getLineJoin() override
	{
	}

	void setLineJoinMiterLimit(float p_miterLimit) override
	{
	}
	float getLineJoinMiterLimit() override
	{
	}

	//------------------------------

	void pushClipGeometry(Avo::Geometry const& p_geometry, float p_opacity) override
	{
	}

	//------------------------------

	void pushClipShape(std::vector<Avo::Point<>> const& p_points, float p_opacity) override
	{
	}
	void pushClipShape(Avo::Point<> const* p_points, uint32 p_numberOfPoints, float p_opacity) override
	{
	}

	void popClipShape() override
	{
		if (!m_clippingShapeStack.empty())
		{
			m_clippingShapeStack.pop();
			if (!m_clippingShapeStack.empty())
			{
				Avo::Rectangle<> bounds = m_clippingShapeStack.top().getBounds();
				glScissor(bounds.left, bounds.top, std::ceil(bounds.getWidth()), std::ceil(bounds.getHeight()));
			}
			else
			{
				glScissor(0, 0, m_size.x*m_dipToPixelFactor, m_size.y*m_dipToPixelFactor);
			}
		}
	}

	//------------------------------

	void pushClipRectangle(float p_left, float p_top, float p_right, float p_bottom, float p_opacity) override
	{
		if (!m_clippingShapeStack.empty())
		{
			Avo::Rectangle<> clippingBounds = m_clippingShapeStack.top().getBounds().bound(p_left, p_top, p_right, p_bottom);
			m_clippingShapeStack.push(ClippingShape(clippingBounds));
			glScissor(clippingBounds.left, clippingBounds.bottom, std::ceil(clippingBounds.getWidth()), std::ceil(clippingBounds.getHeight()));
		}
		else
		{
			m_clippingShapeStack.push(ClippingShape(p_left, p_bottom, p_right, p_bottom));
			glScissor(p_left, p_bottom, std::ceil(p_right - p_left), std::ceil(p_bottom - p_top));
		}
	}
	void pushClipRectangle(Avo::Rectangle<> p_rectangle, float p_opacity) override
	{
		pushClipRectangle(p_rectangle.left, p_rectangle.top, p_rectangle.right, p_rectangle.bottom, p_opacity);
	}
	void pushClipRectangle(Avo::Point<> p_size, float p_opacity) override
	{
		pushClipRectangle(0, 0, p_size.x, p_size.y, p_opacity);
	}

	void pushClipRectangle(float p_left, float p_top, float p_right, float p_bottom, Avo::RectangleCorners const& p_corners, float p_opacity) override
	{
	}
	void pushClipRectangle(Avo::Rectangle<> p_rectangle, Avo::RectangleCorners const& p_corners, float p_opacity) override
	{
	}
	void pushClipRectangle(Avo::Point<> p_size, Avo::RectangleCorners const& p_corners, float p_opacity) override
	{
	}

	//------------------------------

	void pushRoundedClipRectangle(float p_left, float p_top, float p_right, float p_bottom, float p_radius, float p_opacity) override
	{
	}
	void pushRoundedClipRectangle(Avo::Rectangle<> p_rectangle, float p_radius, float p_opacity) override
	{
	}
	void pushRoundedClipRectangle(Avo::Point<> p_size, float p_radius, float p_opacity) override
	{
	}

	//------------------------------

	Avo::Image createRectangleShadowImage(Avo::Point<> p_size, float p_blur, Avo::Color p_color) override
	{
	}
	Avo::Image createRectangleShadowImage(float p_width, float p_height, float p_blur, Avo::Color p_color) override
	{
	}

	Avo::Image createRectangleShadowImage(Avo::Point<> p_size, Avo::RectangleCorners const& p_corners, float p_blur, Avo::Color p_color) override
	{
	}
	Avo::Image createRectangleShadowImage(float p_width, float p_height, Avo::RectangleCorners const& p_corners, float p_blur, Avo::Color p_color) override
	{
	}

	//------------------------------

	Avo::Image createRoundedRectangleShadowImage(Avo::Point<> p_size, float p_radius, float p_blur, Avo::Color p_color) override
	{
	}
	Avo::Image createRoundedRectangleShadowImage(float p_width, float p_height, float p_radius, float p_blur, Avo::Color p_color) override
	{
	}

	//------------------------------

	Avo::Image createImage(uint8 const* p_pixelData, uint32 p_width, uint32 p_height) override
	{
		// Make context usable from this thread (thread might be different)
		glXMakeCurrent(m_server, m_windowHandle, m_context); 

		GLuint texture = 0;
		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p_width, p_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, p_pixelData);
		glGenerateMipmap(GL_TEXTURE_2D);

		return createImageFromImplementation(new OpenGlImage{texture, p_width, p_height, m_dipToPixelFactor});
	}
	Avo::Image createImage(uint8 const* p_imageData, uint32 p_size) override
	{
		png_image pngImage{};
		pngImage.version = PNG_IMAGE_VERSION;
		pngImage.format = PNG_FORMAT_BGRA;

		if (!png_image_begin_read_from_memory(&pngImage, p_imageData, p_size))
		{
			return Avo::Image{};
		}

		std::vector<png_byte> data(PNG_IMAGE_SIZE(pngImage));
		if (!png_image_finish_read(&pngImage, nullptr, data.data(), 0, nullptr))
		{
			return Avo::Image{};
		}

		return createImage(data.data(), pngImage.width, pngImage.height);
	}
	Avo::Image createImage(std::string_view p_filePath) override
	{
		if (!filesystem::is_regular_file(p_filePath))
		{
			return Avo::Image{};
		}
		switch (getImageFormatOfFile(p_filePath))
		{
			case Avo::ImageFormat::Png:
			{
				png_image pngImage{};
				pngImage.version = PNG_IMAGE_VERSION;

				if (!png_image_begin_read_from_file(&pngImage, p_filePath.data()))
				{
					return Avo::Image{};
				}

				pngImage.format = PNG_FORMAT_BGRA;

				std::vector<png_byte> data(PNG_IMAGE_SIZE(pngImage));
				if (!png_image_finish_read(&pngImage, nullptr, data.data(), 0, nullptr))
				{
					return Avo::Image{};
				}

				return createImage(data.data(), pngImage.width, pngImage.height);
			}
			case Avo::ImageFormat::Jpeg:
			{
				FILE* file{fopen(p_filePath.data(), "rb")};
				if (!file)
				{
					return Avo::Image{};
				}

				jpeg_error_mgr errorHandler;
				errorHandler.error_exit = [] (j_common_ptr p_info) -> void {
					p_info->err->output_message(p_info);
				};

				jpeg_decompress_struct decompressor{};
				decompressor.err = jpeg_std_error(&errorHandler);

				jpeg_create_decompress(&decompressor);
				jpeg_stdio_src(&decompressor, file);
				jpeg_read_header(&decompressor, true);

				decompressor.out_color_space = JCS_EXT_BGRA;
				jpeg_start_decompress(&decompressor);

				uint32 rowStride = decompressor.output_width*decompressor.output_components;
				JSAMPARRAY rowBuffer = (*decompressor.mem->alloc_sarray)((j_common_ptr)&decompressor, JPOOL_IMAGE, rowStride, 1);

				std::vector<png_byte> data(rowStride*decompressor.output_height);

				while (decompressor.output_scanline < decompressor.output_height)
				{
					jpeg_read_scanlines(&decompressor, rowBuffer, 1);
					std::memcpy(data.data() + (decompressor.output_scanline - 1)*rowStride, rowBuffer[0], rowStride);
				}

				auto image = createImage(data.data(), decompressor.output_width, decompressor.output_height);

				jpeg_finish_decompress(&decompressor);
				jpeg_destroy_decompress(&decompressor);
				fclose(file);

				return image;
			}
		}
		return Avo::Image{};
	}
	Avo::Image createImageFromHandle(void* p_handle) override
	{
		return Avo::Image{};
	}
	void drawImage(Avo::Image const& p_image, float p_multiplicativeOpacity) override
	{
		Avo::Rectangle<> innerBounds = p_image.getInnerBounds();
		float width = innerBounds.getWidth();
		float height = innerBounds.getHeight();

		Avo::Rectangle<> cropRectangle = p_image.getCropRectangle();
		Avo::Point<> originalSize = p_image.getOriginalSize();
		cropRectangle.left /= originalSize.x;
		cropRectangle.top /= originalSize.y;
		cropRectangle.right /= originalSize.x;
		cropRectangle.bottom /= originalSize.y;

		m_vertexBuffer.insert(
			m_vertexBuffer.end(),
			{
				innerBounds.left , innerBounds.top   , 1.f, width, 0.f, height, cropRectangle.left , cropRectangle.top,
				innerBounds.right, innerBounds.top   , 0.f, width, 0.f, height, cropRectangle.right, cropRectangle.top,
				innerBounds.right, innerBounds.bottom, 0.f, width, 1.f, height, cropRectangle.right, cropRectangle.bottom,

				innerBounds.left , innerBounds.top   , 0.f, width, 1.f, height, cropRectangle.left , cropRectangle.top,
				innerBounds.left , innerBounds.bottom, 0.f, width, 0.f, height, cropRectangle.left , cropRectangle.bottom,
				innerBounds.right, innerBounds.bottom, 1.f, width, 0.f, height, cropRectangle.right, cropRectangle.bottom,
			}
		);
		addDrawCall(6, *(GLuint*)p_image.getHandle());
	}

	//------------------------------

	std::vector<uint8> createImageFileData(Avo::Image const& p_image, Avo::ImageFormat p_format) override
	{
	}
	void* createImageFileDataNativeStream(Avo::Image const& p_image, Avo::ImageFormat p_format) override
	{
	}
	void saveImageToFile(Avo::Image const& p_image, std::string_view p_filePath, Avo::ImageFormat p_format) override
	{
	}

	//------------------------------

	void* createNativeImageFromImage(Avo::Image const& p_image) override
	{
	}

	//------------------------------

	Avo::LinearGradient createLinearGradient(std::vector<Avo::GradientStop> const& p_gradientStops, float p_startX, float p_startY, float p_endX, float p_endY) override
	{
	}
	Avo::LinearGradient createLinearGradient(std::vector<Avo::GradientStop> const& p_gradientStops, Avo::Point<> p_startPosition, Avo::Point<> p_endPosition) override
	{
	}

	Avo::RadialGradient createRadialGradient(std::vector<Avo::GradientStop> const& p_gradientStops, float p_startX, float p_startY, float p_radiusX, float p_radiusY) override
	{
	}
	Avo::RadialGradient createRadialGradient(std::vector<Avo::GradientStop> const& p_gradientStops, float p_startX, float p_startY, float p_radius) override
	{
	}
	Avo::RadialGradient createRadialGradient(std::vector<Avo::GradientStop> const& p_gradientStops, Avo::Point<> p_startPosition, float p_radiusX, float p_radiusY) override
	{
	}
	Avo::RadialGradient createRadialGradient(std::vector<Avo::GradientStop> const& p_gradientStops, Avo::Point<> p_startPosition, float p_radius) override
	{
	}
	Avo::RadialGradient createRadialGradient(std::vector<Avo::GradientStop> const& p_gradientStops, Avo::Point<> p_startPosition, Avo::Point<> p_radius) override
	{
	}

	void setGradient(Avo::LinearGradient const& p_gradient) override
	{
	}
	void setGradient(Avo::RadialGradient const& p_gradient) override
	{
	}
	void setColor(Avo::Color p_color) override
	{
		m_currentColor = p_color;
	}

	void setOpacity(float p_opacity) override
	{
	}

	//------------------------------

	void addFont(std::string_view p_data) override
	{
	}
	void addFont(std::vector<uint8> const& p_data) override
	{
	}
	void addFont(std::vector<uint8>&& p_data) override
	{
	}
	void addFont(uint8 const* p_data, uint32 p_dataSize) override
	{
	}

	//------------------------------

	void setDefaultTextProperties(Avo::TextProperties const& p_textProperties) override
	{
	}
	Avo::TextProperties getDefaultTextProperties() override
	{
	}

	//------------------------------

	Avo::Text createText(std::string_view p_string, float p_fontSize, Avo::Rectangle<> p_bounds) override
	{
	}
	void drawText(Avo::Text const& p_text) override
	{
	}

	void drawText(std::string_view p_string, Avo::Rectangle<> p_rectangle) override
	{
	}
	void drawText(std::string_view p_string, float p_left, float p_top, float p_right, float p_bottom) override
	{
	}
	void drawText(std::string_view p_string, Avo::Point<> p_position, Avo::Point<> p_size) override
	{
	}
	void drawText(std::string_view p_string, float p_x, float p_y) override
	{
	}
	void drawText(std::string_view p_string, Avo::Point<> p_position) override
	{
	}
};

bool OpenGlDrawingContext::s_areStaticVariablesInitialized = false;
Avo::Point<> OpenGlDrawingContext::s_unitCirclePoints[OpenGlDrawingContext::CIRCLE_RESOLUTION + 1];

#endif

//------------------------------
// class GUI
//------------------------------

auto Avo::Gui::getTopMouseListenersAt(Avo::Point<> const p_coordinates) -> std::vector<Avo::View*>
{
	std::vector<Avo::View*> result;
	if (getAreMouseEventsEnabled())
	{
		remember();
		result = {this};
	}
	else
	{
		result = {};
	}
	result.reserve(10);

	Avo::View* container = this;
	Index startIndex = getNumberOfChildViews() - 1;

	bool hasFoundTopView = false;

	while (true)
	{
	loopStart:
		for (Index a = startIndex; a >= 0; a--)
		{
			auto child = container->getChildView(a);
			// Invisible views and their children do not receive mouse events.
			if (child->getIsVisible() && child->getIsContainingAbsolute(p_coordinates))
			{
				bool hasChildren = child->getNumberOfChildViews();

				if (child->getAreMouseEventsEnabled())
				{
					child->remember();
					result.push_back(child);
				}
				if (hasChildren)
				{
					container = child;
					startIndex = container->getNumberOfChildViews() - 1;
					goto loopStart; // I have determined this is the least messy way to do it pls don't kill me
				}
				else
				{
					// We only continue of it's an overlay view, meaning mouse events can pass through it.
					if (!child->getIsOverlay())
					{
						// This is only used to determine if the outer loop should be exited afterwards.
						hasFoundTopView = true;
						break;
					}
				}
			}
		}

		// If an overlay view has children that are not overlay views and one of those are targeted by 
		// the mouse event, mouse events won't pass through!
		if (!container->getIsOverlay() || hasFoundTopView || container == this)
		{
			break;
		}

		startIndex = container->getIndex() - 1;
		container = container->getParent<View>();
	}
	return result;
}

auto Avo::Gui::thread_runAnimationLoop() -> void
{	
	using namespace std::chrono;
	using namespace std::chrono_literals;

	using _Duration = duration<double, std::nano>;
	auto syncInterval = _Duration{1.s/60.};

	auto timeBefore = steady_clock::now();

	bool wasLastFrameDrawn = false;

	while (!m_window->getWillClose())
	{
		lockThreads();
		for (auto a : Indices{m_viewAnimationUpdateQueue})
		{
			m_viewAnimationUpdateQueue.front()->m_isInAnimationUpdateQueue = false;
			m_viewAnimationUpdateQueue.front()->updateAnimations();
			m_viewAnimationUpdateQueue.front()->forget();
			m_viewAnimationUpdateQueue.pop_front();
		}
		unlockThreads();

		lockThreads();
		for (auto a : Indices{m_animationUpdateQueue})
		{
			m_animationUpdateQueue.front()->update();
			m_animationUpdateQueue.pop_front();
		}
		unlockThreads();

		if (!m_invalidRectangles.empty())
		{
			drawViews();

			//wasLastFrameDrawn = true;

			if (!getDrawingContext()->getIsVsyncEnabled())
			{
				std::this_thread::sleep_for(syncInterval);
			}
		}
		else
		{
			//if (wasLastFrameDrawn)
			//{
			//	// Just to force a buffer swap.
			//	invalidateRectangle(0, 0, 1, 1);
			//	drawViews();
			//	wasLastFrameDrawn = false;
			//}
			//else
			//{
				std::this_thread::sleep_for(syncInterval);
			//}
		}
		auto timeAfter = steady_clock::now();
		syncInterval = std::max(_Duration{1.ms}, syncInterval + (1.s/60. - (timeAfter - timeBefore))/2);
		//syncInterval = std::max(1000000, int32(syncInterval + 0.5*(16666667 - (timeAfter - timeBefore).count())));
		timeBefore = timeAfter;
	}
	if (m_viewAnimationUpdateQueue.size())
	{
		while (m_viewAnimationUpdateQueue.size())
		{
			m_viewAnimationUpdateQueue.front()->m_isInAnimationUpdateQueue = false;
			m_viewAnimationUpdateQueue.front()->forget();
			m_viewAnimationUpdateQueue.pop_front();
		}
	}

	// This will cause the window to be destroyed, because getWillClose() is true.
	m_window->close();
	forget();
}

Count Avo::Gui::s_numberOfInstances = 0u;

Avo::Gui::Gui() :
	Avo::View{nullptr}
{
	++s_numberOfInstances;

#ifdef _WIN32
	Direct2dDrawingContext::createStaticResources();
	m_window = std::make_unique<WindowsWindow>(this);
#endif
#ifdef __linux__
	m_window = std::make_unique<LinuxWindow>(this);
#endif

	m_window->windowCreateListeners += Avo::bind(&Gui::handleWindowCreate, this);
	m_window->windowDestroyListeners += Avo::bind(&Gui::handleWindowDestroy, this);
	m_window->windowSizeChangeListeners += Avo::bind(&Gui::handleWindowSizeChange, this);

	m_gui = this;
}
Avo::Gui::Gui(Component* const p_parent) :
	Gui{}
{
	Component::setParent(p_parent);
}
Avo::Gui::~Gui()
{
	--s_numberOfInstances;
}

auto Avo::Gui::create(
	std::string_view const p_title, 
	Avo::Point<Avo::Factor> const p_position, 
	Avo::Size<> const p_size, 
	WindowStyleFlags const p_windowFlags, 
	Gui* const p_parent
) -> void
{
	if (p_parent)
	{
		m_parent = p_parent;
	}

	m_absolutePosition = 0;
	m_window->create(p_title, p_position, p_size, p_windowFlags, p_parent ? p_parent->getWindow() : nullptr);
}

//------------------------------

auto Avo::Gui::getViewAt(Avo::Point<> const p_coordinates) -> Avo::View*
{
	auto currentContainer = static_cast<Avo::View*>(this);
	while (true)
	{
		if (auto const hitViewIterator = std::find_if(
				currentContainer->rbegin(), currentContainer->rend(), 
				[&](View* const view){
					return view->getIsVisible() && !view->getIsOverlay() && view->getIsContainingAbsolute(p_coordinates);
				}
			); hitViewIterator == currentContainer->rend())
		{
			return currentContainer;
		}
		else
		{
			auto* const view = *hitViewIterator;
			if (view->getHasChildViews())
			{
				currentContainer = view;
			}
			else
			{
				return view;
			}
		}
	}

	// while (true)
	// {
	// 	for (auto a = currentContainer->getNumberOfChildViews() - 1; a >= 0; a--)
	// 	{
	// 		auto* const view = currentContainer->getChildView(a);
	// 		if (view->getIsVisible() && !view->getIsOverlay() && view->getIsContainingAbsolute(p_coordinates))
	// 		{
	// 			if (view->getNumberOfChildViews())
	// 			{
	// 				currentContainer = view;
	// 				break;
	// 			}
	// 			else
	// 			{
	// 				return view;
	// 			}
	// 		}
	// 		else if (!a)
	// 		{
	// 			return currentContainer;
	// 		}
	// 	}
	// }
}

//------------------------------

auto Avo::Gui::handleWindowCreate(WindowEvent const& p_event) -> void
{
#ifdef _WIN32
	m_drawingContext = std::make_unique<Direct2dDrawingContext>(m_window.get());
#elif __linux__
	m_drawingContext = std::make_unique<OpenGlDrawingContext>(m_window.get());
#endif
	m_drawingContextState = m_drawingContext->createDrawingState();

	m_bounds = p_event.size;
	m_shadowBounds = m_bounds;
	m_lastUpdatedWindowSize = p_event.size;
}
auto Avo::Gui::handleWindowDestroy(WindowEvent const& p_event) -> void
{
	lockThreads();
	if (!m_viewAnimationUpdateQueue.empty())
	{
		for (auto view : m_viewAnimationUpdateQueue)
		{
			view->forget();
		}
		m_viewAnimationUpdateQueue.clear();
	}
	if (!m_pressedMouseEventListeners.empty())
	{
		for (auto view : m_pressedMouseEventListeners)
		{
			view->forget();
		}
		m_pressedMouseEventListeners.clear();
	}
	unlockThreads();
}
auto Avo::Gui::handleWindowSizeChange(WindowEvent const& p_event) -> void
{
	lockThreads();
	m_drawingContext->setSize(p_event.size);

	m_bounds = p_event.size;
	m_shadowBounds = m_bounds;

	sendBoundsChangeEvents(m_lastUpdatedWindowSize);

	m_lastUpdatedWindowSize = p_event.size;
	m_invalidRectangles.clear();

	invalidate();
	unlockThreads();
}

//------------------------------

auto Avo::Gui::handleGlobalDragDropMove(DragDropEvent& p_event) -> void
{
	auto container = static_cast<Avo::View*>(this);
	auto startIndex = static_cast<Avo::Index>(m_childViews.size()) - 1;

	auto* const thisView = static_cast<Avo::View*>(this);

	auto wasHoveringStack = std::stack<bool>{};
	wasHoveringStack.push(thisView->m_isDraggingOver);

	if (getIsContaining(p_event.xy))
	{
		if (m_areDragDropEventsEnabled)
		{
			if (thisView->m_isDraggingOver)
			{
				dragDropMoveListeners(p_event);
			}
			else
			{
				dragDropEnterListeners(p_event);

				if (startIndex < 0)
				{
					dragDropBackgroundEnterListeners(p_event);
				}
			}
		}
		thisView->m_isDraggingOver = true;
	}
	else if (thisView->m_isDraggingOver)
	{
		if (m_areDragDropEventsEnabled)
		{
			dragDropLeaveListeners(p_event);

			if (startIndex < 0)
			{
				dragDropBackgroundLeaveListeners(p_event);
			}
		}
		thisView->m_isDraggingOver = false;
	}

	auto absolute = p_event.xy;
	if (startIndex >= 0)
	{
		auto hasInvisibleParent = bool{};
		auto hasOverlayParent = bool{};
		auto hasFoundEnterViews = bool{};
		auto hasFoundLeaveViews = bool{};
		while (true)
		{
		loopStart:
			for (auto a = startIndex; a >= 0; a--)
			{
				auto* const child = container->m_childViews[a];

				if (container->m_isDraggingOver && child->getIsContainingAbsolute(absolute) && 
				    child->getIsVisible() && !hasInvisibleParent && !hasFoundEnterViews)
				{
					if (child->m_areDragDropEventsEnabled)
					{
						p_event.xy = absolute - child->getAbsoluteTopLeft();
					}

					auto isContainer = !child->m_childViews.empty();

					if (child->m_isDraggingOver)
					{
						if (child->m_areDragDropEventsEnabled)
						{
							child->dragDropMoveListeners(p_event);
						}
					}
					else
					{
						if (child->m_areDragDropEventsEnabled)
						{
							child->dragDropEnterListeners(p_event);
							if (!isContainer)
							{
								child->dragDropBackgroundEnterListeners(p_event);
							}
						}
					}

					if (isContainer)
					{
						wasHoveringStack.push(child->m_isDraggingOver);
						child->m_isDraggingOver = true;
						if (child->getIsOverlay())
						{
							hasOverlayParent = true;
						}
						container = child;
						startIndex = child->getNumberOfChildViews() - 1;
						goto loopStart;
					}
					else
					{
						if (!hasOverlayParent && !child->getIsOverlay())
						{
							hasFoundEnterViews = true;
							if (child->m_isDraggingOver)
							{
								hasFoundLeaveViews = true;
								break;
							}
							else if (hasFoundLeaveViews)
							{
								child->m_isDraggingOver = true;
								break;
							}
						}
						child->m_isDraggingOver = true;
					}
				}
				else if (child->m_isDraggingOver && !hasFoundLeaveViews)
				{
					auto isContainer = static_cast<bool>(child->getNumberOfChildViews());

					if (child->m_areDragDropEventsEnabled)
					{
						p_event.xy = absolute - child->getAbsoluteTopLeft();
						child->dragDropLeaveListeners(p_event);
						if (!isContainer)
						{
							child->dragDropBackgroundLeaveListeners(p_event);
						}
					}

					if (isContainer)
					{
						wasHoveringStack.push(child->m_isDraggingOver);
						child->m_isDraggingOver = false;

						if (child->m_isOverlay)
						{
							hasOverlayParent = true;
						}
						if (!child->m_isVisible)
						{
							hasInvisibleParent = true;
						}
						container = child;
						startIndex = child->getNumberOfChildViews() - 1;
						goto loopStart;
					}
					else
					{
						child->m_isDraggingOver = false;
						if (!hasOverlayParent && !child->m_isOverlay)
						{
							hasFoundLeaveViews = true;
							if (hasFoundEnterViews)
							{
								break;
							}
						}
					}
				}
			}

			if (wasHoveringStack.top() && container->m_isDraggingOver && hasFoundLeaveViews && !hasFoundEnterViews ||
				!wasHoveringStack.top() && container->m_isDraggingOver && !hasFoundEnterViews)
			{
				hasFoundEnterViews = true;
				if (container->m_areDragDropEventsEnabled)
				{
					p_event.xy = absolute - container->getAbsoluteTopLeft();
					container->dragDropBackgroundEnterListeners(p_event);
				}
			}
			else if (wasHoveringStack.top() && container->m_isDraggingOver && hasFoundEnterViews && !hasFoundLeaveViews ||
			         wasHoveringStack.top() && !container->m_isDraggingOver && !hasFoundLeaveViews)
			{
				hasFoundLeaveViews = true;
				if (container->m_areDragDropEventsEnabled)
				{
					p_event.xy = absolute - container->getAbsoluteTopLeft();
					container->dragDropBackgroundLeaveListeners(p_event);
				}
			}
			else if (wasHoveringStack.top() && container->m_isDraggingOver)
			{
				hasFoundEnterViews = true;
				hasFoundLeaveViews = true;
			}

			if (container == this)
			{
				break;
			}

			if (container->getIsOverlay())
			{
				wasHoveringStack.pop();
				hasOverlayParent = false;
				startIndex = container->getIndex() - 1;
				container = container->getParent<View>();
			}
			else
			{
				while (container != this && wasHoveringStack.top() != container->m_isDraggingOver)
				{
					wasHoveringStack.pop();
					startIndex = container->getIndex() - 1;
					container = container->getParent<View>();
					if (container->getIsOverlay())
					{
						hasOverlayParent = false;
					}
					if (!container->getIsVisible())
					{
						hasInvisibleParent = false;
					}
				}

				if (hasFoundLeaveViews && hasFoundEnterViews)
				{
					break;
				}
			}
		}
	}
	p_event.xy = absolute;
}
auto Avo::Gui::handleGlobalDragDropLeave(DragDropEvent& p_event) -> void
{
	if (m_isDraggingOver)
	{
		if (m_areDragDropEventsEnabled)
		{
			dragDropLeaveListeners(p_event);
			dragDropBackgroundLeaveListeners(p_event);
		}
		m_isDraggingOver = false;
	}

	auto const absolute = p_event.xy;

	Avo::View* container = this;
	Index startIndex = m_childViews.size() - 1;
	Count numberOfOverlayParents = 0;
	while (true)
	{
	loopStart:
		for (Index a = startIndex; a >= 0; a--)
		{
			Avo::View* child = container->m_childViews[a];

			if (child->m_isDraggingOver)
			{
				if (child->m_areDragDropEventsEnabled)
				{
					p_event.xy = absolute - child->getAbsoluteTopLeft();
					child->dragDropLeaveListeners(p_event);
					child->dragDropBackgroundLeaveListeners(p_event);
				}
				child->m_isDraggingOver = false;

				if (!child->m_childViews.empty())
				{
					startIndex = child->m_childViews.size() - 1;
					container = child;
					if (container->m_isOverlay)
					{
						numberOfOverlayParents++;
					}
					goto loopStart;
				}
				else if (!child->m_isOverlay)
				{
					break;
				}
			}
		}
		if (container == this)
		{
			break;
		}
		else if (container->m_isOverlay)
		{
			startIndex = container->m_index - 1;
			container = container->m_parent;
			numberOfOverlayParents--;
		}
		else if (numberOfOverlayParents)
		{
			while (!container->m_isOverlay && container != this)
			{
				container = container->m_parent;
			}
			if (container == this)
			{
				break;
			}
			startIndex = container->m_index - 1;
			container = container->m_parent;
			numberOfOverlayParents--;
		}
		else
		{
			break;
		}
	}
}

//------------------------------

auto Avo::Gui::handleGlobalMouseMove(MouseEvent p_event) -> void
{
	// This is false if it's called from a view just to send mouse leave and mouse enter events,
	// if a view has been moved from the mouse for example.
	auto const wasMouseReallyMoved = bool{p_event.movement};

	auto const absolute = p_event.xy;

	if (!m_pressedMouseEventListeners.empty())
	{
		if (wasMouseReallyMoved)
		{
			for (auto* const pressedView : m_pressedMouseEventListeners)
			{
				p_event.xy = absolute - pressedView->getAbsoluteTopLeft();
				pressedView->mouseMoveListeners(p_event);
			}
		}
	}
	else
	{
		Avo::View* container = this;
		Index startIndex = m_childViews.size() - 1;

		auto* const thisView = static_cast<View*>(this);

		std::stack<bool> wasHoveringStack;
		wasHoveringStack.push(thisView->m_isMouseHovering);

		if (getIsContaining(p_event.xy))
		{
			if (m_areMouseEventsEnabled)
			{
				if (thisView->m_isMouseHovering)
				{
					if (wasMouseReallyMoved)
					{
						mouseMoveListeners(p_event);
					}
				}
				else
				{
					mouseEnterListeners(p_event);

					if (startIndex < 0)
					{
						mouseBackgroundEnterListeners(p_event);
					}
				}
			}
			thisView->m_isMouseHovering = true;
		}
		else if (thisView->m_isMouseHovering)
		{
			if (getAreMouseEventsEnabled())
			{
				mouseLeaveListeners(p_event);

				if (startIndex < 0)
				{
					mouseBackgroundLeaveListeners(p_event);
				}
			}
			thisView->m_isMouseHovering = false;
		}

		if (startIndex >= 0)
		{
			bool hasInvisibleParent = false;
			bool hasOverlayParent = false;
			bool hasFoundEnterViews = false;
			bool hasFoundLeaveViews = false;
			while (true)
			{
			loopStart:
				for (Index a = startIndex; a >= 0; a--)
				{
					auto child = container->m_childViews[a];

					if (container->m_isMouseHovering && child->getIsContainingAbsolute(absolute) && 
					    child->m_isVisible && !hasInvisibleParent && !hasFoundEnterViews)
					{
						if (child->m_areMouseEventsEnabled)
						{
							p_event.xy = absolute - child->getAbsoluteTopLeft();
						}

						bool isContainer = !child->m_childViews.empty();

						if (child->m_isMouseHovering)
						{
							if (child->m_areMouseEventsEnabled && wasMouseReallyMoved)
							{
								child->mouseMoveListeners(p_event);
							}
						}
						else
						{
							if (child->m_areMouseEventsEnabled)
							{
								child->mouseEnterListeners(p_event);
								if (!isContainer)
								{
									child->mouseBackgroundEnterListeners(p_event);
								}
							}
						}

						if (isContainer)
						{
							wasHoveringStack.push(child->m_isMouseHovering);
							child->m_isMouseHovering = true;
							if (child->m_isOverlay)
							{
								hasOverlayParent = true;
							}
							container = child;
							startIndex = child->getNumberOfChildViews() - 1;
							goto loopStart;
						}
						else
						{
							if (!hasOverlayParent && !child->m_isOverlay)
							{
								hasFoundEnterViews = true;
								if (child->m_isMouseHovering)
								{
									hasFoundLeaveViews = true;
									break;
								}
								else if (hasFoundLeaveViews)
								{
									child->m_isMouseHovering = true;
									break;
								}
							}
							child->m_isMouseHovering = true;
						}
					}
					else if (child->m_isMouseHovering && !hasFoundLeaveViews)
					{
						bool isContainer = child->getNumberOfChildViews();

						if (child->m_areMouseEventsEnabled)
						{
							p_event.xy = absolute - child->getAbsoluteTopLeft();
							child->mouseLeaveListeners(p_event);
							if (!isContainer)
							{
								child->mouseBackgroundLeaveListeners(p_event);
							}
						}

						if (isContainer)
						{
							wasHoveringStack.push(child->m_isMouseHovering);
							child->m_isMouseHovering = false;

							if (child->m_isOverlay)
							{
								hasOverlayParent = true;
							}
							if (!child->m_isVisible)
							{
								hasInvisibleParent = true;
							}
							container = child;
							startIndex = child->getNumberOfChildViews() - 1;
							goto loopStart;
						}
						else
						{
							child->m_isMouseHovering = false;
							if (!hasOverlayParent && !child->m_isOverlay)
							{
								hasFoundLeaveViews = true;
								if (hasFoundEnterViews)
								{
									break;
								}
							}
						}
					}
				}

				if (wasHoveringStack.top() && container->m_isMouseHovering && hasFoundLeaveViews && !hasFoundEnterViews ||
					!wasHoveringStack.top() && container->m_isMouseHovering && !hasFoundEnterViews)
				{
					hasFoundEnterViews = true;
					if (container->m_areMouseEventsEnabled)
					{
						p_event.xy = absolute - container->getAbsoluteTopLeft();
						container->mouseBackgroundEnterListeners(p_event);
					}
				}
				else if (wasHoveringStack.top() && container->m_isMouseHovering && hasFoundEnterViews && !hasFoundLeaveViews ||
					wasHoveringStack.top() && !container->m_isMouseHovering && !hasFoundLeaveViews)
				{
					hasFoundLeaveViews = true;
					if (container->m_areMouseEventsEnabled)
					{
						p_event.xy = absolute - container->getAbsoluteTopLeft();
						container->mouseBackgroundLeaveListeners(p_event);
					}
				}
				else if (wasHoveringStack.top() && container->m_isMouseHovering)
				{
					hasFoundEnterViews = true;
					hasFoundLeaveViews = true;
				}

				if (container == this)
				{
					break;
				}

				if (container->m_isOverlay)
				{
					wasHoveringStack.pop();
					hasOverlayParent = false;
					startIndex = container->getIndex() - 1;
					container = container->getParent<View>();
				}
				else
				{
					while (container != this && wasHoveringStack.top() != container->m_isMouseHovering)
					{
						wasHoveringStack.pop();
						startIndex = container->getIndex() - 1;
						container = container->getParent<View>();
						if (container->m_isOverlay)
						{
							hasOverlayParent = false;
						}
						if (!container->m_isVisible)
						{
							hasInvisibleParent = false;
						}
					}

					if (hasFoundLeaveViews && hasFoundEnterViews)
					{
						break;
					}
				}
			}
		}
	}
}
auto Avo::Gui::handleGlobalMouseLeave(MouseEvent p_event) -> void
{
	if (!m_pressedMouseEventListeners.empty())
	{
		return;
	}

	if (m_isMouseHovering)
	{
		if (m_areMouseEventsEnabled)
		{
			mouseLeaveListeners(p_event);
			mouseBackgroundLeaveListeners(p_event);
		}
		m_isMouseHovering = false;
	}

	auto const absolute = p_event.xy;

	Avo::View* container = this;
	Index startIndex = m_childViews.size() - 1;
	Count numberOfOverlayParents = 0;
	while (true)
	{
		loopStart:
		for (Index a = startIndex; a >= 0; a--)
		{
			Avo::View* const child = container->m_childViews[a];

			if (child->m_isMouseHovering)
			{
				if (child->m_areMouseEventsEnabled)
				{
					p_event.xy = absolute - child->getAbsoluteTopLeft();
					child->mouseLeaveListeners(p_event);
					child->mouseBackgroundLeaveListeners(p_event);
				}
				child->m_isMouseHovering = false;

				if (child->m_childViews.size())
				{
					startIndex = child->m_childViews.size() - 1;
					container = child;
					if (container->m_isOverlay)
					{
						numberOfOverlayParents++;
					}
					goto loopStart;
				}
				else if (!child->m_isOverlay)
				{
					break;
				}
			}
		}
		if (container == this)
		{
			break;
		}
		else if (container->m_isOverlay)
		{
			startIndex = container->m_index - 1;
			container = container->m_parent;
			numberOfOverlayParents--;
		}
		else if (numberOfOverlayParents)
		{
			while (!container->m_isOverlay && container != this)
			{
				container = container->m_parent;
			}
			if (container == this)
			{
				break;
			}
			startIndex = container->m_index - 1;
			container = container->m_parent;
			numberOfOverlayParents--;
		}
		else
		{
			break;
		}
	}
}

//------------------------------

auto Avo::Gui::invalidateRectangle(Avo::Rectangle<> p_rectangle) -> void
{
	p_rectangle.bound(m_bounds);

	if (p_rectangle.getWidth() == 0.f || p_rectangle.getHeight() == 0.f)
	{
		return;
	}

	//------------------------------
	// Here, we're rounding the coordinates to whole device pixels so that anti aliased clipping doesn't leave traces at the edges.

	float const dipToPixelFactor = m_window->getDipToPixelFactor();
	p_rectangle = {
		floor(p_rectangle.left*dipToPixelFactor)/dipToPixelFactor,
		floor(p_rectangle.top*dipToPixelFactor)/dipToPixelFactor,
		ceil(p_rectangle.right*dipToPixelFactor)/dipToPixelFactor,
		ceil(p_rectangle.bottom*dipToPixelFactor)/dipToPixelFactor
	};

	//------------------------------

	Index rectangleIndex = -1;
	Avo::Rectangle<>* rectangle = nullptr;

	bool willAdd = true;
	bool isDone = false;
	while (!isDone)
	{
		if (rectangle)
		{
			isDone = true;
			auto lock = std::scoped_lock{m_invalidRectanglesMutex};
			for (auto a : Indices{m_invalidRectangles})
			{
				if (a != rectangleIndex)
				{
					if (m_invalidRectangles[a].getIsIntersecting(*rectangle))
					{
						m_invalidRectangles[a].contain(*rectangle);
						m_invalidRectangles.erase(m_invalidRectangles.begin() + rectangleIndex);
						if (rectangleIndex < a)
						{
							a--;
						}
						rectangle = &m_invalidRectangles[a];
						rectangleIndex = a;
						isDone = false;
						break;
					}
				}
			}
		}
		else
		{
			isDone = true;
			auto lock = std::scoped_lock{m_invalidRectanglesMutex};
			for (auto a : Indices{m_invalidRectangles})
			{
				if (m_invalidRectangles[a].getIsIntersecting(p_rectangle))
				{
					rectangleIndex = a;
					rectangle = m_invalidRectangles.data() + a;
					rectangle->contain(p_rectangle);
					willAdd = false;
					isDone = false;
					break;
				}
			}
		}
	}
	if (willAdd)
	{
		auto lock = std::scoped_lock{m_invalidRectanglesMutex};
		m_invalidRectangles.push_back(p_rectangle);
	}
}

auto Avo::Gui::drawViews() -> void
{
	if (!m_invalidRectangles.empty())
	{
		m_invalidRectanglesMutex.lock();
		auto invalidRectangles = std::move(m_invalidRectangles);
		m_invalidRectangles = std::vector<Avo::Rectangle<>>();
		m_invalidRectanglesMutex.unlock();

		lockThreads(); // State needs to be static during drawing.

		m_drawingContext->beginDrawing();

		for (auto const& targetRectangle : invalidRectangles)
		{
			auto currentContainer = static_cast<Avo::View*>(this);
			auto startPosition = Index{};

			m_drawingContext->resetTransformations();
			m_drawingContext->setOpacity(1.f);
			m_drawingContext->pushClipRectangle(targetRectangle);

			m_drawingContext->clear(m_theme->colors[ThemeColors::background]);

			draw(m_drawingContext.get(), targetRectangle);

			while (true)
			{
				bool isDoneWithContainer = true;
				for (auto a : Indices{startPosition, currentContainer->getNumberOfChildViews()})
				{
					auto view = currentContainer->getChildView(a);

					if (view->getSize() > 0.f && view->getIsVisible())
					{
						if (view->getAbsoluteBounds().getIsIntersecting(targetRectangle) && 
							view->getIsIntersecting(currentContainer->getSize()))
						{
							m_drawingContext->resetTransformations();
							m_drawingContext->setOrigin(view->getAbsoluteTopLeft());
							m_drawingContext->setOpacity(1.f);

							view->drawShadow(m_drawingContext.get());

							auto& corners = view->getCorners();
							if (view->getHasCornerStyles())
							{
								m_drawingContext->pushClipGeometry(view->m_clipGeometry, view->m_opacity);
							}
							else
							{
								m_drawingContext->pushClipRectangle(view->getSize(), view->m_opacity);
							}

							view->draw(m_drawingContext.get(), targetRectangle);

							if (view->getNumberOfChildViews())
							{
								currentContainer = view;
								startPosition = 0;
								isDoneWithContainer = false;
								break;
							}
							else
							{
								m_drawingContext->resetTransformations();
								m_drawingContext->setOrigin(view->getAbsoluteTopLeft());
								m_drawingContext->setOpacity(1.f);
								view->drawOverlay(m_drawingContext.get(), targetRectangle);

								m_drawingContext->popClipShape();
							}
						}
						else if (view->getAbsoluteShadowBounds().getIsIntersecting(targetRectangle))
						{
							m_drawingContext->resetTransformations();
							m_drawingContext->setOrigin(view->getAbsoluteTopLeft());
							m_drawingContext->setOpacity(1.f);
							view->drawShadow(m_drawingContext.get());
						}
					}
				}
				if (isDoneWithContainer)
				{
					if (currentContainer == this)
					{
						break;
					}

					m_drawingContext->resetTransformations();
					m_drawingContext->setOrigin(currentContainer->getAbsoluteTopLeft());
					m_drawingContext->setOpacity(1.f);
					currentContainer->drawOverlay(m_drawingContext.get(), targetRectangle);

					m_drawingContext->popClipShape();

					startPosition = currentContainer->getIndex() + 1U;
					currentContainer = currentContainer->getParent<View>();
				}
			}

			m_drawingContext->resetTransformations();
			m_drawingContext->setOpacity(1.f);
			drawOverlay(m_drawingContext.get(), targetRectangle);
			m_drawingContext->popClipShape();
		}
		unlockThreads();
		m_drawingContext->restoreDrawingState(m_drawingContextState);
		m_drawingContext->finishDrawing(invalidRectangles);
	}
}

//------------------------------
// class Avo::OpenFileDialog
//------------------------------

auto Avo::OpenFileDialog::open() -> std::vector<std::string>
{
#ifdef _WIN32
	auto dialog = ComReference<IFileOpenDialog>{};
	CoCreateInstance(CLSID_FileOpenDialog, 0, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&dialog));

	auto wideTitle = std::array<char16, 256>{};
	convertUtf8ToUtf16(m_title, wideTitle);
	dialog->SetTitle(reinterpret_cast<LPCWSTR>(wideTitle.data()));

	//------------------------------
	// Create the extension filters that the user will choose from.

	auto filters = std::vector<COMDLG_FILTERSPEC>(m_fileExtensions.size());

	constexpr auto maxFilterStringLength = 100;
	constexpr auto halfMaxFilterStringLength = maxFilterStringLength/2;

	// I made 1 big string buffer to decrease the number of allocations.
	auto filterStringBuffer = std::vector<wchar_t>(maxFilterStringLength*m_fileExtensions.size());
	for (auto a : Indices{m_fileExtensions})
	{
		auto start = filterStringBuffer.data() + a*maxFilterStringLength;
		convertUtf8ToUtf16(m_fileExtensions[a].name, {reinterpret_cast<char16*>(start), halfMaxFilterStringLength});
		filters[a].pszName = start;

		start = filterStringBuffer.data() + a*maxFilterStringLength + 50;
		convertUtf8ToUtf16(m_fileExtensions[a].extensions, {reinterpret_cast<char16*>(start), halfMaxFilterStringLength});
		filters[a].pszSpec = start;
	}
	dialog->SetFileTypes(m_fileExtensions.size(), filters.data());

	//------------------------------

	if (m_canSelectMultipleFiles)
	{
		auto options = FILEOPENDIALOGOPTIONS{};
		dialog->GetOptions(&options);
		dialog->SetOptions(options | FOS_ALLOWMULTISELECT);
	}

	//------------------------------

	auto result = std::vector<std::string>();
	if (SUCCEEDED(dialog->Show(m_gui ? static_cast<HWND>(m_gui->getWindow()->getNativeHandle()) : nullptr)))
	{
		if (m_canSelectMultipleFiles)
		{
			auto items = ComReference<IShellItemArray>{};
			dialog->GetResults(&items);

			auto numberOfResults = DWORD{};
			items->GetCount(&numberOfResults);
			result.resize(numberOfResults);

			for (auto resultIndex : Range{numberOfResults})
			{
				auto item = ComReference<IShellItem>{};
				items->GetItemAt(resultIndex, &item);

				auto name = LPWSTR{};
				item->GetDisplayName(SIGDN_FILESYSPATH, &name);
				if (name)
				{
					result[resultIndex] = Avo::convertUtf16ToUtf8(reinterpret_cast<char16 const*>(name));
					CoTaskMemFree(name);
				}
			}
		}
		else
		{
			auto item = ComReference<IShellItem>{};
			dialog->GetResult(&item);

			auto name = LPWSTR{};
			item->GetDisplayName(SIGDN_FILESYSPATH, &name);
			result.resize(1);
			if (name)
			{
				result[0] = Avo::convertUtf16ToUtf8(reinterpret_cast<char16 const*>(name));
				CoTaskMemFree(name);
			}
		}
	}
	return result;
#endif
}
