#ifndef SIMQ_UTIL_SORTED_ARRAY 
#define SIMQ_UTIL_SORTED_ARRAY 

#include "error.h"
#include <string.h>
#include <mutex>
#include <type_traits>
#include <iostream>
#include <stdlib.h>

namespace simq::util {
    template<typename Tt, typename T>


    class SortedArray {
        private:
            std::mutex m;
            const unsigned int THRESHOLD_REMOVE = 10'000;
            struct Item {
                Tt name;
                T t;
                bool isLive;
            };

            enum SearchDataType {
                ISSET,
                NONE,
                TO_START,
                TO_END,
            };

            struct SearchData {
                unsigned int offset;
                SearchDataType type;
            };

            Item *getNewItem( Tt name, T t );
            Item **items = nullptr;
            unsigned int itemsLength = 0;
            unsigned int countRemoved = 0;

            void search( Tt name, SearchData &data );
        public:
            SortedArray() {
                constexpr bool isNormalType = std::is_same<Tt, int>::value || std::is_same<Tt, unsigned int>::value || std::is_same<Tt, const char *>::value || std::is_same<Tt, char *>::value;
                static_assert( isNormalType, "Error type");
            }
            void push( Tt name, T t );
            void get( Tt name );
            void remove( Tt name );
            ~SortedArray();
    };

    template<typename Tt, typename T>
    void SortedArray<Tt, T>::remove( Tt name ) {
        std::lock_guard<std::mutex> lock( m );

        SearchData data;
        search( name, data );

        if( data.type == SearchDataType::ISSET && items[data.offset]->isLive ) {
            items[data.offset]->isLive = false;
            countRemoved++;
        }
    }

    template<typename Tt, typename T>
    void SortedArray<Tt, T>::push( Tt name, T t ) {
        std::lock_guard<std::mutex> lock( m );

        bool isNone = false;

        SearchData data;
        search( name, data );

        if( data.type != SearchDataType::ISSET ) {
            isNone = true;
        } else if( items[data.offset]->isLive ) {
            throw simq::util::Error::IS_EXISTS;
        }


        if( isNone ) {
            auto *item = getNewItem( name, t );

            if( itemsLength == 0 ) {
                items = (Item **)malloc( sizeof( Item * ) );
                items[0] = item;
                itemsLength++;
            } else {
                items = (Item **)realloc( item, sizeof( Item * ) * ( itemsLength + 1 ) );

                if( data.type == SearchDataType::TO_START ) {
                    memmove( &items[1], items, sizeof( Item * ) * itemsLength );
                    items[0] = item;
                } else if( data.type == SearchDataType::TO_END ) {
                    items[itemsLength] = item;
                } else {
                    memmove( &items[data.offset+1], items[data.offset], sizeof( Item * ) * ( itemsLength - data.offset ));
                    items[data.offset] = item;
                }

                
                /*
        if( s_data.offset == TO_START ) {
            memmove( &this->list[1], this->list, sizeof( data ) * length );
            this->list[0].id = id;
            this->list[0].data = add_data;
        } else if( s_data.offset == TO_END ) {
            this->list[this->length].id = id;
            this->list[this->length].data = add_data;
        } else {
            memmove( &this->list[s_data.offset+1], &this->list[s_data.offset], sizeof( data ) * ( length - s_data.offset ));
            this->list[s_data.offset].id = id;
            this->list[s_data.offset].data = add_data;
        }
        */

                itemsLength++;
            }
        } else {
            items[data.offset]->isLive = true;
            items[data.offset]->t = t;
            countRemoved--;
        }
    }

    template<typename Tt, typename T>
    typename SortedArray<Tt, T>::Item *SortedArray<Tt, T>::getNewItem( Tt name, T t ) {
        Item *item = new Item{};
        memcpy( &item->t, &t, sizeof( T ) );

        if constexpr (std::is_same<Tt, int>::value || std::is_same<Tt, unsigned int>::value ) {
            item->name = name;
        } else {
            auto length = strlen( name );
            item->name = new char[length+1]{};
            memcpy( (void *)item->name, (void *)name, length );
        }

        item->isLive = true;

        return item;
    }

    template<typename Tt, typename T>
    void SortedArray<Tt, T>::get( Tt name ) {
        std::lock_guard<std::mutex> lock( m );

        SearchData data;
        std::cout << "before get" << std::endl;
        search( name, data );
        std::cout << "get" << std::endl;

        if( data.type != SearchDataType::ISSET ) {
            throw simq::util::Error::NOT_FOUND;
        }

        std::cout << "get" << std::endl;

        //return &items[data.offset]->t;
    }


    template<typename Tt, typename T>
    void SortedArray<Tt, T>::search( Tt name, SearchData &data ) {
        unsigned int min = 0;
        unsigned int max = itemsLength;
        unsigned int mid;
        data.type = SearchDataType::NONE;
        data.offset = 0;

        if( max == 0 ) {
            return;
        }

        if constexpr (std::is_same<Tt, int>::value || std::is_same<Tt, unsigned int>::value ) {
            while( min <= max ) {
                mid = (min + max) / 2;
                if( mid == itemsLength ) {
                    break;
                }

                if( items[mid]->name > name ) {
                    max = mid - 1;
                } else if( items[mid]->name < name ) {
                    min = mid + 1;
                } else {
                    data.type = ISSET;
                    data.offset = mid;
                    return;
                }
            }
        } else {
            while( min <= max ) {
                mid = (min + max) / 2;
                if( mid == itemsLength ) {
                    break;
                }

                if( strcmp( items[mid]->name, name ) > 0 ) {
                    max = mid - 1;
                } else if( strcmp( items[mid]->name, name ) < 0 ) {
                    min = mid + 1;
                } else {
                    data.type = ISSET;
                    data.offset = mid;
                    return;
                }
            }
        }


        if( max == -1 ) {
            data.type = SearchDataType::TO_START;
        } else if( min == max ) {
            data.type = SearchDataType::TO_END;
        } else {
            data.offset = min;
        }

    }

    template<typename Tt, typename T>
    SortedArray<Tt, T>::~SortedArray() {
    }

}

/*
 *
 * void gsx::Array<T>::search( gsx::uint id, search_data *s_data ) {
    int min = 0;
    int max = this->length;
    int mid;

    if( max == 0 ) {
        s_data->pos = -1;
        return;
    }

    while( min <= max ) {
        mid = (min + max) / 2;
        if( mid == length ) {
            break;
        }

        if( this->list[mid].id > id ) {
            max = mid - 1;
        } else if( this->list[mid].id < id ) {
            min = mid + 1;
        } else {
            s_data->pos = mid;
            return;
        }
    }

    if( max == -1 ) {
        s_data->offset = TO_START;
    } else if( min == max ) {
        s_data->offset = TO_END;
    } else {
        s_data->offset = min;
    }

    s_data->pos = -1;
}
*/

#endif
