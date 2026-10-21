# Third-Party Software

RadixLab is released under the MIT License. It does not vendor third-party
source code. The build and runtime dependencies below remain under their own
licenses.

| Component | Role | License |
| --- | --- | --- |
| [Boost.Multiprecision](https://www.boost.org/) | Arbitrary-precision integers | Boost Software License 1.0 |
| [Spring Boot](https://spring.io/projects/spring-boot) | Backend runtime and dependency management | Apache License 2.0 |
| [Spring Framework](https://spring.io/projects/spring-framework) | REST controllers, dependency injection | Apache License 2.0 |
| [Apache Tomcat](https://tomcat.apache.org/) | Embedded HTTP server | Apache License 2.0 |
| [Jackson](https://github.com/FasterXML/jackson) | JSON serialization and parsing | Apache License 2.0 |
| [Hibernate Validator](https://hibernate.org/validator/) | Jakarta Bean Validation implementation | Apache License 2.0 |
| [JUnit 5](https://junit.org/junit5/) | Test framework (test scope) | Eclipse Public License 2.0 |
| [AssertJ](https://assertj.github.io/doc/) | Test assertions (test scope) | Apache License 2.0 |
| [Mockito](https://site.mockito.org/) | Test doubles (test scope) | MIT License |
| [Apache Maven](https://maven.apache.org/) | Java build tool | Apache License 2.0 |
| [CMake](https://cmake.org/) | C++ build tool | BSD 3-Clause License |
| [OpenJDK](https://openjdk.org/) | Java runtime and toolchain | GPLv2 with Classpath Exception |

The project's MIT license is compatible with distributing the project source
alongside these dependencies. If a packaged Spring Boot executable is
redistributed, preserve the license notices inside its dependency jars and
comply with each dependency's terms.

This inventory is provided for engineering clarity, not as legal advice.
