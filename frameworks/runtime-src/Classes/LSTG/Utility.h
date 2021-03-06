﻿/// @file Utility.h
/// @brief 实用工具
#pragma once
#include "Global.h"

namespace lstg
{
	/// @brief 域
	class Scope
	{
	private:
		std::function<void()> m_WhatToDo;
	public:
		Scope& operator=(const Scope&) = delete;
		Scope(const Scope&) = delete;
		Scope(std::function<void()> exitJob)
			: m_WhatToDo(exitJob) {}
		~Scope()
		{
			m_WhatToDo();
		}
	};

	/// @brief 计时域
	class TimerScope
	{
	private:
		fcyStopWatch m_StopWatch;
		float& m_Out;
	public:
		TimerScope(float& Out)
			: m_Out(Out)
		{
		}
		~TimerScope()
		{
			m_Out = static_cast<float>(m_StopWatch.GetElapsed());
		}
	};

	/// @brief 字符串格式化
	/// @param Format 字符串格式，不支持精度
	std::string StringFormat(const char* Format, ...)noexcept;

	/** @brief 字符串格式化 va_list版本\n
	* @code
	* bool     b; int32_t   c; unsigned int p;
	* int32_t  d; uint32_t  u; double  f;
	* int64_t ld; uint64_t lu; double lf;
	* char*    s; wchar_t*  w; @endcode
	*/
	std::string StringFormatV(const char* Format, va_list vaptr)noexcept;

	/// @brief 字符串格式化 宽字符
	/// @param Format 字符串格式，不支持精度
	std::wstring StringFormat(const wchar_t* Format, ...)noexcept;

	/** @brief 字符串格式化 宽字符、va_list版本\n
	* @code
	* bool     b; int32_t   c; unsigned int p;
	* int32_t  d; uint32_t  u; double  f;
	* int64_t ld; uint64_t lu; double lf;
	* char*    m; wchar_t*  s; @endcode
	*/
	std::wstring StringFormatV(const wchar_t* Format, va_list vaptr)noexcept;

	/// @brief 打印Lua堆栈
	void stackDump(lua_State *L);

	std::string ReplacePathSep(std::string path, const std::string& ori = "\\", const std::string& dst = "/");

	cocos2d::Vec2 AlignmentToAnchorPoint(cocos2d::TextHAlignment ha, cocos2d::TextVAlignment va);

	void pathUniform(std::string& path, bool forward_slash = false, bool lower = true, bool utf8_convert = false);
	void pathUniform(std::wstring& path, bool forward_slash = false, bool lower = true, bool utf8_convert = false);
	//
	std::string glBlend_tostring(GLenum blend);

	std::string tostring(const cocos2d::Vec2& v);
	std::string tostring(const cocos2d::Vec3& v);
	std::string tostring(const cocos2d::Vec4& v);
	std::string tostring(const cocos2d::Size& s);
	std::string tostring(const cocos2d::Rect& r);
	std::string tostring(const cocos2d::Color3B& c);
	std::string tostring(const cocos2d::Color4B& c);
	std::string tostring(const cocos2d::Color4F& c);
	std::string tostring(const cocos2d::BlendFunc& b);
	std::string tostring(const cocos2d::V3F_C4B_T2F& v);
	std::string tostring(const cocos2d::V3F_C4B_T2F_Quad& v);
	std::string tostring(const cocos2d::Mat4& m);
	std::string tostring(const cocos2d::TextHAlignment ha);
	std::string tostring(const cocos2d::TextVAlignment va);

	void getNodeTransform(const cocos2d::Vec2& anchorPointInPoints,
		float x, float y, float rot, float hscale, float vscale, float z, cocos2d::Mat4* out);
	cocos2d::Mat4 getNodeTransform(const cocos2d::Vec2& anchorPointInPoints,
		float x, float y, float rot, float hscale, float vscale, float z);

	void getQuaternion(const cocos2d::Vec3& axis, float angle, cocos2d::Quaternion* out);

	void getRotationTransform(const cocos2d::Quaternion& q, cocos2d::Mat4* out);
	void getRotationTransform(const cocos2d::Vec3& axis, float angle, cocos2d::Mat4* out);
	
	void getNodeTransform3D(const cocos2d::Vec2& anchorPointInPoints,
		float x, float y, float z, const cocos2d::Quaternion& q,
		float hscale, float vscale, float zscale, cocos2d::Mat4* out);
	void getNodeTransform3D(const cocos2d::Vec2& anchorPointInPoints,
		float x, float y, float z, const cocos2d::Vec3& axis, float angle,
		float hscale, float vscale, float zscale, cocos2d::Mat4* out);

	cocos2d::Image* GetTextureImage(cocos2d::Texture2D* texture, bool flipImage);

	// (start, end)
	void deployThreadTask(size_t taskSize, size_t nSlice, const std::function<void(int, int)>& task);
	// (start, end, iThread)
	void deployThreadTask(size_t taskSize, size_t nSlice, const std::function<void(int, int, int)>& task);

	// RC4 cryption
	class RC4
	{
	private:
		uint8_t S[256];
	public:
		void operator()(const uint8_t* input, size_t inputlen, uint8_t* output);
		RC4(const uint8_t* password, size_t len);
	};

	namespace util
	{
		bool glMacroFromString(const std::string& str, GLuint& val);
		cocos2d::GLProgram* CreateGLProgramFromPath(const std::string& v, const std::string& f);
		cocos2d::GLProgram* CreateGLProgramFromString(const std::string& v, const std::string& f);
		cocos2d::GLProgram* CreateGLProgramFromData(cocos2d::Data* v, cocos2d::Data* f);
		bool CheckFragmentShader(const std::string& src);
		bool CheckVertexShader(const std::string& src);
		bool CheckBlendEquation(GLuint equation);
		bool CheckBlendFunc(GLenum func);
	}
}
