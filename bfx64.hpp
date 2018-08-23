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

#include <cstring>
#include <algorithm>
#include <sstream>
#include <string_view>
#include <vector>

#include <sys/mman.h>

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
			if (!m_loops.empty())
			{
				throw std::out_of_range {"invalid loop"};
			}

			m_stream
				<< "    mov   $0,       %rax"       << '\n'
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

		void emit_write()
		{
			m_stream
				<< "    push  %rdi"               << '\n'
				<< "    mov   $1,       %rdx"     << '\n' // count
				<< "    mov   %rdi,     %rsi"     << '\n' // buffer
				<< "    mov   $1,       %rdi"     << '\n' // file descriptor
				<< "    mov   $1,       %rax"     << '\n' // sys_write
				<< "    syscall"                  << '\n'
				<< "    pop   %rdi"               << '\n';
		}

		void emit_read()
		{
			m_stream
				<< "    push  %rdi"               << '\n'
				<< "    mov   $1,       %rdx"     << '\n' // count
				<< "    mov   %rdi,     %rsi"     << '\n' // buffer
				<< "    mov   $0,       %rdi"     << '\n' // file descriptor
				<< "    mov   $0,       %rax"     << '\n' // sys_read
				<< "    syscall"                  << '\n'
				<< "    pop   %rdi"               << '\n';
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

	class BinaryGenerator
	{
	public:
		explicit BinaryGenerator()
			: m_code()
		{
			m_code.reserve(1024);

			m_code.insert(std::end(m_code), {
				// push  %rbp
				'\x55',
				// mov %rsp, %rbp
				'\x48', '\x89', '\xe5',
				// sub $0xffff, %rsp
				'\x48', '\x81', '\xec', '\xff', '\xff', '\x00', '\x00',
				// mov %rsp, %rdi
				'\x48', '\x89', '\xe7',
				// mov $0, (%rdi)
				'\x48', '\xc7', '\x07', '\x00', '\x00', '\x00', '\x00',
				// add $8, %rdi
				'\x48', '\x83', '\xc7', '\x08',
				// cmp %rbp, %rdi
				'\x48', '\x39', '\xef',
				// jg -0x10
				'\x7f', '\xf0',
				// mov %rsp, %rdi
				'\x48', '\x89', '\xe7'
			});
		}

		void finish()
		{
			if (!m_loops.empty())
			{
				throw std::out_of_range {"invalid loop"};
			}

			m_code.insert(std::end(m_code), {
				// mov %rbp, %rsp
				'\x48', '\x89', '\xec',
				// pop %rbp
				'\x5d',
				// ret
				'\xc3',
			});
		}

		void emit_next()
		{
			m_code.insert(std::end(m_code), {
				// add $1,%rdi
				'\x48', '\x83', '\xc7', '\x01',
			});
		}

		void emit_prev()
		{
			m_code.insert(std::end(m_code), {
				// sub $1,%rdi
				'\x48', '\x83', '\xef', '\x01',
			});
		}

		void emit_increment()
		{
			m_code.insert(std::end(m_code), {
				// movzb (%rdi), %rax
				'\x48', '\x0f', '\xb6', '\x07',
				// add $1, %rax
				'\x48', '\x83', '\xc0', '\x01',
				// movb %al, (%rdi)
				'\x88', '\x07',
			});
		}

		void emit_decrement()
		{
			m_code.insert(std::end(m_code), {
				// movzb (%rdi), %rax
				'\x48', '\x0f', '\xb6', '\x07',
				// sub $1, %rax
				'\x48', '\x83', '\xe8', '\x01',
				// movb %al, (%rdi)
				'\x88', '\x07',
			});
		}

		void emit_loop()
		{
			m_loops.emplace_back(m_code.size());

			m_code.insert(std::end(m_code), {
				// movzb (%rdi), %rax
				'\x48', '\x0f', '\xb6', '\x07',
				// sub $0, %rax
				'\x48', '\x83', '\xf8', '\x00',
				// je .BOTTOM
				'\x0f', '\x84', '\x00', '\x00', '\x00', '\x00'
				// '\x90', '\xe9', '\x00', '\x00', '\x00', '\x00'
			});
		}

		void emit_loop_end()
		{
			if (m_loops.empty())
			{
				throw std::out_of_range {"invalid loop"};
			}

			const auto loop = m_loops.back();
			const std::size_t diff = loop - m_code.size() - 5;
			const std::size_t patch = m_code.size() - loop - 9;

			m_loops.pop_back();

			m_code.insert(std::end(m_code), {
				// jmp .TOP
				'\xe9',
				static_cast<char>(diff >>  0),
				static_cast<char>(diff >>  8),
				static_cast<char>(diff >> 16),
				static_cast<char>(diff >> 24),
			});

			m_code[loop + 10] = static_cast<char>(patch >>  0);
			m_code[loop + 11] = static_cast<char>(patch >>  8);
			m_code[loop + 12] = static_cast<char>(patch >> 16);
			m_code[loop + 13] = static_cast<char>(patch >> 24);
		}

		void emit_write()
		{
			m_code.insert(std::end(m_code), {
				// push %rdi
				'\x57',
				// mov $1,%rdx
				'\x48', '\xc7', '\xc2', '\x01', '\x00', '\x00', '\x00',
				// mov %rdi,%rsi
				'\x48', '\x89', '\xfe',
				// mov $1,%rdi
				'\x48', '\xc7', '\xc7', '\x01', '\x00', '\x00', '\x00',
				// mov $1,%rax
				'\x48', '\xc7', '\xc0', '\x01', '\x00', '\x00', '\x00',
				// syscall
				'\x0f', '\x05',
				// pop %rdi
				'\x5f',
			});
		}

		void emit_read()
		{
			m_code.insert(std::end(m_code), {
				// push %rdi
				'\x57',
				// mov $1,%rdx
				'\x48', '\xc7', '\xc2', '\x01', '\x00', '\x00', '\x00',
				// mov %rdi,%rsi
				'\x48', '\x89', '\xfe',
				// mov $0,%rdi
				'\x48', '\xc7', '\xc7', '\x00', '\x00', '\x00', '\x00',
				// mov $0,%rax
				'\x48', '\xc7', '\xc0', '\x00', '\x00', '\x00', '\x00',
				// syscall
				'\x0f', '\x05',
				// pop    %rdi
				'\x5f',
			});
		}

		// Uncopyable, unmovable.
		BinaryGenerator(const BinaryGenerator&) =delete;
		BinaryGenerator(BinaryGenerator&&) =delete;

		BinaryGenerator& operator=(const BinaryGenerator&) =delete;
		BinaryGenerator& operator=(BinaryGenerator&&) =delete;

		[[nodiscard]]
		void (*code())() const
		// std::vector<char> code() const
		{
			if (void* mem = mmap(nullptr, m_code.size(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS ,-1, 0); mem != MAP_FAILED)
			{
				std::memcpy(mem, m_code.data(), m_code.size());

				// return m_code;
				return reinterpret_cast<void(*)()>(mem);
			}

			throw std::bad_alloc {};
		}

	private:
		std::vector<char> m_code;
		std::vector<std::size_t> m_loops;
	};

	template <typename CodeGenerator>
	[[nodiscard]]
	decltype(auto) compile(std::string_view source)
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

				case '.':
					gen.emit_write();
					break;

				case ',':
					gen.emit_read();
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
