#define _WIN32_WINNT 0x0501
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/asio/io_service.hpp>
#include <boost/asio/windows/overlapped_ptr.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_array.hpp>
#include <iostream>

// ----- I/O object

class directory_monitor
{
public:
	directory_monitor(boost::asio::io_service &io_service, const boost::filesystem::path &directory)
		: handle_(io_service)
	{
		boost::system::error_code ec;

		HANDLE handle = CreateFileW(directory.wstring().c_str(), FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);

		if (handle == INVALID_HANDLE_VALUE)
		{
			ec.assign(::GetLastError(), boost::system::system_category());
		}
		else
		{
			handle_.assign(handle, ec);
			if (ec)
				CloseHandle(handle);
		}

		if (ec)
			throw boost::system::system_error(ec);
	}

	void close()
	{
		handle_.close();
	}

	template <typename Handler>
	void async_wait(Handler handler)
	{
		wait_op<Handler> op =
		{
			handler,
			boost::shared_array<unsigned char>(new unsigned char[16384])
		};

		boost::asio::windows::overlapped_ptr overlapped(handle_.get_io_service(), op);

		DWORD length = 0;
		if (!ReadDirectoryChangesW(handle_.native(), op.buffer_.get(), 16384,
			TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION
			| FILE_NOTIFY_CHANGE_FILE_NAME, &length, overlapped.get(), 0))
		{
			boost::system::error_code ec(::GetLastError(), boost::system::system_category());
			overlapped.complete(ec, 0);
		}
		else
		{
			overlapped.release();
		}
	}

private:
	template <typename Handler>
	struct wait_op
	{
		Handler handler_;
		boost::shared_array<unsigned char> buffer_;

		void operator()(boost::system::error_code ec, std::size_t bytes_transferred)
		{
			int action = 0;
			boost::filesystem::path path;

			if (bytes_transferred)
			{
                // TODO: Check whether there are multiple events.
				const FILE_NOTIFY_INFORMATION *fni =
					reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer_.get());
				action = fni->Action;
				path.assign(fni->FileName, fni->FileName + fni->FileNameLength / 2);
			}
			else if (!ec)
			{
				ec = boost::asio::error::operation_aborted;
			}

			handler_(ec, action, path);
		}
	};

	boost::asio::windows::stream_handle handle_;
};

// ----- main

void wait_handler(const boost::system::error_code &ec, int action, const boost::filesystem::path &path)
{
	std::cout << "ec = " << ec.message() << std::endl;
	std::cout << "action = " << action << std::endl;
	std::cout << "path = " << path << std::endl;
}

int main()
{
	boost::asio::io_service io_service;
	directory_monitor monitor(io_service, ".");
	monitor.async_wait(wait_handler);
	io_service.run();
	std::cin.get();
}
