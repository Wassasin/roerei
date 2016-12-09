#pragma once

#include <ostream>

namespace roerei {
	class csv_dump
	{
	private:
		std::ostream& os;
		bool ended;

	public:
		csv_dump(std::ostream& _os)
		: os(_os)
		, ended(false)
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
			if(!ended) {
				os.flush();
				ended = true;
			}
		}

		~csv_dump()
		{
			write_end();
		}
	};
}
