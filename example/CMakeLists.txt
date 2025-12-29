add_executable(doublebuf_example source/main.cpp)
target_link_libraries(doublebuf_example PRIVATE doublebuf)

if(NOT MSVC)
  target_compile_options(doublebuf_example PRIVATE -Wall -Wextra -Wconversion)

  # sanitizer
  target_compile_options(doublebuf_example PRIVATE -fsanitize=thread)
  target_link_options(doublebuf_example PRIVATE -fsanitize=thread)
else()
  target_compile_options(doublebuf_example PRIVATE /W4)
endif()
