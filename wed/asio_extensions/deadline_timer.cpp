#define _WIN32_WINNT 0x0501
#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <iostream>
#include <cstddef>

// ----- I/O object

template <typename Service = boost::asio::deadline_timer_service<boost::posix_time::ptime> >
class deadline_timer
	: public boost::asio::basic_io_object<Service>
{
public:
	explicit deadline_timer(boost::asio::io_service &io_service)
		: boost::asio::basic_io_object<Service>(io_service)
	{
	}

	void wait(std::size_t sec)
	{
		boost::system::error_code ec;
		this->service.expires_from_now(this->implementation, boost::posix_time::seconds(sec), ec);
		this->service.wait(this->implementation, ec);
	}

	template <typename Handler>
	void async_wait(std::size_t sec, Handler handler)
	{
		boost::system::error_code ec;
		this->service.expires_from_now(this->implementation, boost::posix_time::seconds(sec), ec);
		this->service.async_wait(this->implementation, handler);
	}
};

// ----- main

void handler(const boost::system::error_code &ec)
{
	std::cout << "handler (" << ec << ")" << std::endl;
}

int main()
{
	boost::asio::io_service ioservice;
	deadline_timer<> dt(ioservice);
	dt.async_wait(3, handler);
	ioservice.run();
	std::cin.get();
}
