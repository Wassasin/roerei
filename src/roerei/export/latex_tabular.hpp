#pragma once

#include <ostream>

namespace roerei {
	class latex_tabular
	{
	private:
		std::ostream& os;

	public:
		latex_tabular(std::ostream& _os)
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
					os << " & ";
				}

				os << col;
			}

			os << "\\\\\n";
		}

		void write_end()
		{
			os.flush();
		}

		~latex_tabular()
		{
			write_end();
		}
	};
}
