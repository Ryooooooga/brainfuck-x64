/*================================================================================
 * The MIT License
 *
 * Copyright (c) 2018 Ryooooooga
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
================================================================================*/

#ifndef INCLUDE_BFX64_HPP
#define INCLUDE_BFX64_HPP

#include <algorithm>
#include <sstream>
#include <string_view>
#include <vector>

namespace bfx64
{
	class AssemblyGenerator
	{
	public:
		explicit AssemblyGenerator()
			: m_stream()
		{
			m_stream
				<< "    .global  main"          << '\n'
				<< "    .section .text"         << '\n'
				<< "main:"                      << '\n'
				<< "    push  %rbp"             << '\n'
				<< "    mov   %rsp,     %rbp"   << '\n'
				<< "    sub   $0xffff,  %rsp"   << '\n'
				<< "    lea   1(%rbp),  %rdi"   << '\n'
				<< "    movb  $0,       (%rdi)" << '\n'
				;
		}

		void finish()
		{
			m_stream
				<< "    movzb (%rdi),   %rax"   << '\n'
				<< "    mov   %rbp,     %rsp"   << '\n'
				<< "    pop   %rbp"             << '\n'
				<< "    ret"                    << '\n';
		}

		void emit_increment()
		{
			m_stream
				<< "    movzb (%rdi),   %rax"   << '\n'
				<< "    add   $1,       %rax"   << '\n'
				<< "    movb  %al,      (%rdi)" << '\n';
		}

		void emit_decrement()
		{
			m_stream
				<< "    movzb (%rdi),   %rax"   << '\n'
				<< "    sub   $1,       %rax"   << '\n'
				<< "    movb  %al,      (%rdi)" << '\n';
		}

		// Uncopyable, unmovable.
		AssemblyGenerator(const AssemblyGenerator&) =delete;
		AssemblyGenerator(AssemblyGenerator&&) =delete;

		AssemblyGenerator& operator=(const AssemblyGenerator&) =delete;
		AssemblyGenerator& operator=(AssemblyGenerator&&) =delete;

		[[nodiscard]]
		std::string code() const
		{
			return m_stream.str();
		}

	private:
		std::string m_entry;
		std::ostringstream m_stream;
	};

	template <typename CodeGenerator>
	[[nodiscard]]
	auto compile(std::string_view source)
	{
		std::vector<std::size_t> loops;
		loops.reserve(std::count(std::begin(source), std::end(source), '['));

		CodeGenerator gen;

		for (const auto c : source)
		{
			switch (c)
			{
				case '+':
					gen.emit_increment();
					break;

				case '-':
					gen.emit_decrement();
					break;

				default:
					break;
			}
		}

		gen.finish();

		return gen.code();
	}
}

#endif
