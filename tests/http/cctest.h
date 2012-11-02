// Copyright 2008 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef CCTEST_H_
#define CCTEST_H_

#include <stddef.h>

// The normal strdup functions use malloc.  These versions of StrDup
// and StrNDup uses new and calls the FatalProcessOutOfMemory handler
// if allocation fails.
char* StrDup(const char* str);
char* StrNDup(const char* str, int n);


#ifndef TEST
#define TEST(Name)                                                       \
  static void Test##Name();                                              \
  CcTest register_test_##Name(Test##Name, __FILE__, #Name, NULL, true);  \
  static void Test##Name()
#endif

#ifndef DEPENDENT_TEST
#define DEPENDENT_TEST(Name, Dep)                                        \
  static void Test##Name();                                              \
  CcTest register_test_##Name(Test##Name, __FILE__, #Name, #Dep, true);  \
  static void Test##Name()
#endif

#ifndef DISABLED_TEST
#define DISABLED_TEST(Name)                                              \
  static void Test##Name();                                              \
  CcTest register_test_##Name(Test##Name, __FILE__, #Name, NULL, false); \
  static void Test##Name()
#endif

class CcTest {
 public:
  typedef void (TestFunction)();
  CcTest(TestFunction* callback, const char* file, const char* name,
         const char* dependency, bool enabled);
  void Run() { callback_(); }
  static int test_count();
  static CcTest* last() { return last_; }
  CcTest* prev() { return prev_; }
  const char* file() { return file_; }
  const char* name() { return name_; }
  const char* dependency() { return dependency_; }
  bool enabled() { return enabled_; }
 private:
  TestFunction* callback_;
  const char* file_;
  const char* name_;
  const char* dependency_;
  bool enabled_;
  static CcTest* last_;
  CcTest* prev_;
};

// // Switches between all the Api tests using the threading support.
// // In order to get a surprising but repeatable pattern of thread
// // switching it has extra semaphores to control the order in which
// // the tests alternate, not relying solely on the big V8 lock.
// //
// // A test is augmented with calls to ApiTestFuzzer::Fuzz() in its
// // callbacks.  This will have no effect when we are not running the
// // thread fuzzing test.  In the thread fuzzing test it will
// // pseudorandomly select a successor thread and switch execution
// // to that thread, suspending the current test.
// class ApiTestFuzzer: public v8::internal::Thread {
//  public:
//   void CallTest();
//   explicit ApiTestFuzzer(int num)
//       : Thread("ApiTestFuzzer"),
//         test_number_(num),
//         gate_(v8::internal::OS::CreateSemaphore(0)),
//         active_(true) {
//   }
//   ~ApiTestFuzzer() { delete gate_; }

//   // The ApiTestFuzzer is also a Thread, so it has a Run method.
//   virtual void Run();

//   enum PartOfTest { FIRST_PART,
//                     SECOND_PART,
//                     THIRD_PART,
//                     FOURTH_PART,
//                     LAST_PART = FOURTH_PART };

//   static void SetUp(PartOfTest part);
//   static void RunAllTests();
//   static void TearDown();
//   // This method switches threads if we are running the Threading test.
//   // Otherwise it does nothing.
//   static void Fuzz();

//  private:
//   static bool fuzzing_;
//   static int tests_being_run_;
//   static int current_;
//   static int active_tests_;
//   static bool NextThread();
//   int test_number_;
//   v8::internal::Semaphore* gate_;
//   bool active_;
//   void ContextSwitch();
//   static int GetNextTestNumber();
//   static v8::internal::Semaphore* all_tests_done_;
// };


// #define THREADED_TEST(Name)                                          \
//   static void Test##Name();                                          \
//   RegisterThreadedTest register_##Name(Test##Name, #Name);           \
//   /* */ TEST(Name)


// class RegisterThreadedTest {
//  public:
//   explicit RegisterThreadedTest(CcTest::TestFunction* callback,
//                                 const char* name)
//       : fuzzer_(NULL), callback_(callback), name_(name) {
//     prev_ = first_;
//     first_ = this;
//     count_++;
//   }
//   static int count() { return count_; }
//   static RegisterThreadedTest* nth(int i) {
//     CHECK(i < count());
//     RegisterThreadedTest* current = first_;
//     while (i > 0) {
//       i--;
//       current = current->prev_;
//     }
//     return current;
//   }
//   CcTest::TestFunction* callback() { return callback_; }
//   ApiTestFuzzer* fuzzer_;
//   const char* name() { return name_; }

//  private:
//   static RegisterThreadedTest* first_;
//   static int count_;
//   CcTest::TestFunction* callback_;
//   RegisterThreadedTest* prev_;
//   const char* name_;
// };


// // A LocalContext holds a reference to a v8::Context.
// class LocalContext {
//  public:
//   LocalContext(v8::ExtensionConfiguration* extensions = 0,
//                v8::Handle<v8::ObjectTemplate> global_template =
//                    v8::Handle<v8::ObjectTemplate>(),
//                v8::Handle<v8::Value> global_object = v8::Handle<v8::Value>())
//     : context_(v8::Context::New(extensions, global_template, global_object)) {
//     context_->Enter();
//   }

//   virtual ~LocalContext() {
//     context_->Exit();
//     context_.Dispose();
//   }

//   v8::Context* operator->() { return *context_; }
//   v8::Context* operator*() { return *context_; }
//   bool IsReady() { return !context_.IsEmpty(); }

//   v8::Local<v8::Context> local() {
//     return v8::Local<v8::Context>::New(context_);
//   }

//  private:
//   v8::Persistent<v8::Context> context_;
// };


// static inline v8::Local<v8::Value> v8_num(double x) {
//   return v8::Number::New(x);
// }


// static inline v8::Local<v8::String> v8_str(const char* x) {
//   return v8::String::New(x);
// }


// static inline v8::Local<v8::Script> v8_compile(const char* x) {
//   return v8::Script::Compile(v8_str(x));
// }


// // Helper function that compiles and runs the source.
// static inline v8::Local<v8::Value> CompileRun(const char* source) {
//   return v8::Script::Compile(v8::String::New(source))->Run();
// }


#include <string.h>
#include <stdint.h>

extern "C" void V8_Fatal(const char* file, int line, const char* format, ...);

// The FATAL, UNREACHABLE and UNIMPLEMENTED macros are useful during
// development, but they should not be relied on in the final product.
#ifdef DEBUG
#define FATAL(msg)                              \
  V8_Fatal(__FILE__, __LINE__, "%s", (msg))
#define UNIMPLEMENTED()                         \
  V8_Fatal(__FILE__, __LINE__, "unimplemented code")
#define UNREACHABLE()                           \
  V8_Fatal(__FILE__, __LINE__, "unreachable code")
#else
#define FATAL(msg)                              \
  V8_Fatal("", 0, "%s", (msg))
#define UNIMPLEMENTED()                         \
  V8_Fatal("", 0, "unimplemented code")
#define UNREACHABLE() ((void) 0)
#endif


// The CHECK macro checks that the given condition is true; if not, it
// prints a message to stderr and aborts.
#define CHECK(condition) do {                                       \
    if (!(condition)) {                                             \
      V8_Fatal(__FILE__, __LINE__, "CHECK(%s) failed", #condition); \
    }                                                               \
  } while (0)


// Helper function used by the CHECK_EQ function when given int
// arguments.  Should not be called directly.
inline void CheckEqualsHelper(const char* file, int line,
                              const char* expected_source, int expected,
                              const char* value_source, int value) {
  if (expected != value) {
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#   Expected: %i\n#   Found: %i",
             expected_source, value_source, expected, value);
  }
}


// Helper function used by the CHECK_EQ function when given int64_t
// arguments.  Should not be called directly.
inline void CheckEqualsHelper(const char* file, int line,
                              const char* expected_source,
                              int64_t expected,
                              const char* value_source,
                              int64_t value) {
  if (expected != value) {
    // Print int64_t values in hex, as two int32s,
    // to avoid platform-dependencies.
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#"
             "   Expected: 0x%08x%08x\n#   Found: 0x%08x%08x",
             expected_source, value_source,
             static_cast<uint32_t>(expected >> 32),
             static_cast<uint32_t>(expected),
             static_cast<uint32_t>(value >> 32),
             static_cast<uint32_t>(value));
  }
}


// Helper function used by the CHECK_NE function when given int
// arguments.  Should not be called directly.
inline void CheckNonEqualsHelper(const char* file,
                                 int line,
                                 const char* unexpected_source,
                                 int unexpected,
                                 const char* value_source,
                                 int value) {
  if (unexpected == value) {
    V8_Fatal(file, line, "CHECK_NE(%s, %s) failed\n#   Value: %i",
             unexpected_source, value_source, value);
  }
}


// Helper function used by the CHECK function when given string
// arguments.  Should not be called directly.
inline void CheckEqualsHelper(const char* file,
                              int line,
                              const char* expected_source,
                              const char* expected,
                              const char* value_source,
                              const char* value) {
  if ((expected == NULL && value != NULL) ||
      (expected != NULL && value == NULL) ||
      (expected != NULL && value != NULL && strcmp(expected, value) != 0)) {
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#   Expected: %s\n#   Found: %s",
             expected_source, value_source, expected, value);
  }
}


inline void CheckNonEqualsHelper(const char* file,
                                 int line,
                                 const char* expected_source,
                                 const char* expected,
                                 const char* value_source,
                                 const char* value) {
  if (expected == value ||
      (expected != NULL && value != NULL && strcmp(expected, value) == 0)) {
    V8_Fatal(file, line, "CHECK_NE(%s, %s) failed\n#   Value: %s",
             expected_source, value_source, value);
  }
}


// Helper function used by the CHECK function when given pointer
// arguments.  Should not be called directly.
inline void CheckEqualsHelper(const char* file,
                              int line,
                              const char* expected_source,
                              const void* expected,
                              const char* value_source,
                              const void* value) {
  if (expected != value) {
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#   Expected: %p\n#   Found: %p",
             expected_source, value_source,
             expected, value);
  }
}


inline void CheckNonEqualsHelper(const char* file,
                                 int line,
                                 const char* expected_source,
                                 const void* expected,
                                 const char* value_source,
                                 const void* value) {
  if (expected == value) {
    V8_Fatal(file, line, "CHECK_NE(%s, %s) failed\n#   Value: %p",
             expected_source, value_source, value);
  }
}


// Helper function used by the CHECK function when given floating
// point arguments.  Should not be called directly.
inline void CheckEqualsHelper(const char* file,
                              int line,
                              const char* expected_source,
                              double expected,
                              const char* value_source,
                              double value) {
  // Force values to 64 bit memory to truncate 80 bit precision on IA32.
  volatile double* exp = new double[1];
  *exp = expected;
  volatile double* val = new double[1];
  *val = value;
  if (*exp != *val) {
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#   Expected: %f\n#   Found: %f",
             expected_source, value_source, *exp, *val);
  }
  delete[] exp;
  delete[] val;
}


inline void CheckNonEqualsHelper(const char* file,
                                 int line,
                                 const char* expected_source,
                                 double expected,
                                 const char* value_source,
                                 double value) {
  // Force values to 64 bit memory to truncate 80 bit precision on IA32.
  volatile double* exp = new double[1];
  *exp = expected;
  volatile double* val = new double[1];
  *val = value;
  if (*exp == *val) {
    V8_Fatal(file, line,
             "CHECK_NE(%s, %s) failed\n#   Value: %f",
             expected_source, value_source, *val);
  }
  delete[] exp;
  delete[] val;
}


#define CHECK_EQ(expected, value) CheckEqualsHelper(__FILE__, __LINE__, \
  #expected, expected, #value, value)


#define CHECK_NE(unexpected, value) CheckNonEqualsHelper(__FILE__, __LINE__, \
  #unexpected, unexpected, #value, value)


#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))


// This is inspired by the static assertion facility in boost.  This
// is pretty magical.  If it causes you trouble on a platform you may
// find a fix in the boost code.
template <bool> class StaticAssertion;
template <> class StaticAssertion<true> { };
// This macro joins two tokens.  If one of the tokens is a macro the
// helper call causes it to be resolved before joining.
#define SEMI_STATIC_JOIN(a, b) SEMI_STATIC_JOIN_HELPER(a, b)
#define SEMI_STATIC_JOIN_HELPER(a, b) a##b
// Causes an error during compilation of the condition is not
// statically known to be true.  It is formulated as a typedef so that
// it can be used wherever a typedef can be used.  Beware that this
// actually causes each use to introduce a new defined type with a
// name depending on the source line.
template <int> class StaticAssertionHelper { };
#define STATIC_CHECK(test)                                                    \
  typedef                                                                     \
    StaticAssertionHelper<sizeof(StaticAssertion<static_cast<bool>((test))>)> \
    SEMI_STATIC_JOIN(__StaticAssertTypedef__, __LINE__)


extern bool FLAG_enable_slow_asserts;


// The ASSERT macro is equivalent to CHECK except that it only
// generates code in debug builds.
#ifdef DEBUG
#define ASSERT_RESULT(expr)    CHECK(expr)
#define ASSERT(condition)      CHECK(condition)
#define ASSERT_EQ(v1, v2)      CHECK_EQ(v1, v2)
#define ASSERT_NE(v1, v2)      CHECK_NE(v1, v2)
#define ASSERT_GE(v1, v2)      CHECK_GE(v1, v2)
#define ASSERT_LT(v1, v2)      CHECK_LT(v1, v2)
#define ASSERT_LE(v1, v2)      CHECK_LE(v1, v2)
#define SLOW_ASSERT(condition) CHECK(!FLAG_enable_slow_asserts || (condition))
#else
#define ASSERT_RESULT(expr)    (expr)
#define ASSERT(condition)      ((void) 0)
#define ASSERT_EQ(v1, v2)      ((void) 0)
#define ASSERT_NE(v1, v2)      ((void) 0)
#define ASSERT_GE(v1, v2)      ((void) 0)
#define ASSERT_LT(v1, v2)      ((void) 0)
#define ASSERT_LE(v1, v2)      ((void) 0)
#define SLOW_ASSERT(condition) ((void) 0)
#endif
// Static asserts has no impact on runtime performance, so they can be
// safely enabled in release mode. Moreover, the ((void) 0) expression
// obeys different syntax rules than typedef's, e.g. it can't appear
// inside class declaration, this leads to inconsistency between debug
// and release compilation modes behavior.
#define STATIC_ASSERT(test)  STATIC_CHECK(test)

#define ASSERT_NOT_NULL(p)  ASSERT_NE(NULL, p)


#endif  // ifndef CCTEST_H_
