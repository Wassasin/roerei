#pragma once

#include <ostream>

namespace roerei {
	class data_dump
	{
	private:
		std::ostream& os;

	public:
		data_dump(std::ostream& _os)
		: os(_os)
		{}

		template<typename COL>
		void write_row(std::initializer_list<COL> const& cols)
		{
			bool first = true;
			for(auto const& col : cols) {
				if(first) {
					first = false;
				} else {
					os << '\t';
				}

				os << col;
			}

			os << "\n";
		}

		void write_end()
		{
			os.flush();
		}

		~data_dump()
		{
			write_end();
		}
	};
}
