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
			, m_label(0)
		{
			m_stream
				<< "    .global  main"              << '\n'
				<< "    .section .text"             << '\n'
				<< "main:"                          << '\n'
				<< "    push  %rbp"                 << '\n'
				<< "    mov   %rsp,     %rbp"       << '\n'
				<< "    sub   $0xffff,  %rsp"       << '\n'
				<< "    mov   %rsp,     %rdi"       << '\n'
				<< ".LX:"                           << '\n'
				<< "    movq  $0,       (%rdi)"     << '\n'
				<< "    add   $8,       %rdi"       << '\n'
				<< "    cmp   %rbp,     %rdi"       << '\n'
				<< "    jg    .LX"                  << '\n'
				<< "    mov   %rsp,     %rdi"       << '\n';
		}

		void finish()
		{
			m_stream
				<< "    movzb (%rdi),   %rax"       << '\n'
				<< "    mov   %rbp,     %rsp"       << '\n'
				<< "    pop   %rbp"                 << '\n'
				<< "    ret"                        << '\n';
		}

		void emit_next()
		{
			m_stream
				<< "    add   $1,       %rdi"       << '\n';
		}

		void emit_prev()
		{
			m_stream
				<< "    sub   $1,       %rdi"       << '\n';
		}

		void emit_increment()
		{
			m_stream
				<< "    movzb (%rdi),   %rax"       << '\n'
				<< "    add   $1,       %rax"       << '\n'
				<< "    movb  %al,      (%rdi)"     << '\n';
		}

		void emit_decrement()
		{
			m_stream
				<< "    movzb (%rdi),   %rax"     << '\n'
				<< "    sub   $1,       %rax"     << '\n'
				<< "    movb  %al,      (%rdi)"   << '\n';
		}

		void emit_loop()
		{
			Loop loop =
			{
				m_label++,
				m_label++,
			};

			m_loops.emplace_back(loop);

			m_stream
				<< ".L" << loop.top << ":"        << '\n'
				<< "    movzb (%rdi),   %rax"     << '\n'
				<< "    cmp   $0,       %rax"     << '\n'
				<< "    je    .L" << loop.end     << '\n';
		}

		void emit_loop_end()
		{
			if (m_loops.empty())
			{
				throw std::out_of_range {"invalid loop"};
			}

			const auto loop = m_loops.back();

			m_loops.pop_back();

			m_stream
				<< "    jmp   .L" << loop.top     << '\n'
				<< ".L" << loop.end << ":"        << '\n';
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
		struct Loop
		{
			std::size_t top;
			std::size_t end;
		};

		std::ostringstream m_stream;
		std::vector<Loop> m_loops;
		std::size_t m_label;
	};

	template <typename CodeGenerator>
	[[nodiscard]]
	auto compile(std::string_view source)
	{
		CodeGenerator gen;

		for (const auto c : source)
		{
			switch (c)
			{
				case '<':
					gen.emit_prev();
					break;

				case '>':
					gen.emit_next();
					break;

				case '+':
					gen.emit_increment();
					break;

				case '-':
					gen.emit_decrement();
					break;

				case '[':
					gen.emit_loop();
					break;

				case ']':
					gen.emit_loop_end();
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
