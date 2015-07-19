
#ifndef STABLE_PRIORITY_QUEUE_H_
#define STABLE_PRIORITY_QUEUE_H_

template<class T, class Container = std::list<T>, class Compare = std::less<typename Container::value_type> >
class stable_priority_queue
{
	public:
		typedef typename Container::value_type value_type;
		typedef typename Container::size_type size_type;
		typedef Container container_type;

	protected:
		Container c;
		Compare comp;

	private:
		template<class InputIterator>
			void push(InputIterator first, InputIterator end)
			{
				for( ; first != end ; ++first ) push(*first);
			}

	public:
		explicit stable_priority_queue( const Compare& cp = Compare(), const Container& cc = Container() )
			: comp(cp)
		{
			push(cc.begin(), cc.end());
		}

		bool empty() const { return c.empty(); }
		size_type size() const { return c.size(); }
		const value_type& top() const { return c.back(); }
		void pop() { c.pop_back(); }

		void push( const value_type& x)
		{
			c.insert(std::lower_bound(c.begin(), c.end(), x, comp), x);
		}
};

#endif /* STABLE_PRIORITY_QUEUE_H_ */

