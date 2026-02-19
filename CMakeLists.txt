cmake_minimum_required(VERSION 3.0)

SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

project(finalProject)


#add_definitions(-DSUPPORT_OPENGL_45)

add_executable(
  ${PROJECT_NAME}
  src/main.cpp
  src/Shader.cpp
  src/gl_utils.cpp
  src/Camera.cpp
  src/Spline.cpp
  src/Mesh.cpp
  src/Instancing.cpp
  src/CityBuilder.cpp
  src/Particles.cpp
  src/PostProcess.cpp
  src/ShadowMap.cpp
  src/gltf/GltfModel.cpp
  src/Skybox.cpp
  src/Texture2D.cpp
  src/Raytrace.cpp
  src/PhysicsWorld.cpp
  src/GeometryDemo.cpp
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/dep/tinygltf
)

add_subdirectory(dep/glad)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

add_subdirectory(dep/glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

add_subdirectory(dep/glm)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)

target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})

find_package(Bullet REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${BULLET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${BULLET_LIBRARIES})

add_custom_command(TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_SOURCE_DIR})