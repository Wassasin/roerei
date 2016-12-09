#pragma once

#include <ostream>

namespace roerei {
	class csv_dump
	{
	private:
		std::ostream& os;

	public:
		csv_dump(std::ostream& _os)
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
					os << ',';
				}

				os << col;
			}

			os << "\n";
		}

		void write_end()
		{
			os.flush();
		}

		~csv_dump()
		{
			write_end();
		}
	};
}
