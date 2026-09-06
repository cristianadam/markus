#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <sys/wait.h>
#endif

#include "gtest/gtest.h"
#include "markus.h"

namespace {

std::string GetMainBinaryPath() {
  const char* path = std::getenv("MARKUS_MAIN");
  if (path && path[0] != '\0') {
    return path;
  }
#ifdef __APPLE__
  return "./build/main";
#else
  return "./build/main";
#endif
}

std::string GetSpecPath() {
  const char* path = std::getenv("MARKUS_SPEC");
  if (path && path[0] != '\0') {
    return path;
  }
  return "commonmark-spec/test/spec.txt";
}

std::string RunPythonTest(int test_number, const std::string& main_path,
                          const std::string& spec_path) {
  std::ostringstream cmd;
  cmd << "python3 commonmark-spec/test/spec_tests.py"
      << " --program " << main_path << " -s " << spec_path << " -n "
      << test_number << " 2>&1";

  FILE* pipe = popen(cmd.str().c_str(), "r");
  if (!pipe) return "ERROR: Failed to start python3";

  std::string output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    std::string line(buffer);
    if (line.find(" passed, ") != std::string::npos &&
        line.find(" failed, ") != std::string::npos &&
        line.find(" skipped") != std::string::npos) {
      continue;
    }
    output += line;
  }

  int exit_code;
#ifdef _WIN32
  exit_code = pclose(pipe);
#else
  int status = pclose(pipe);
  if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  } else {
    return "ERROR: pclose failed";
  }
#endif

  if (exit_code == 0) {
    return "PASS";
  } else if (exit_code == 124) {
    return "TIMEOUT";
  }
  std::ostringstream err;
  err << "EXIT_CODE=" << exit_code << "\n" << output;
  return err.str();
}

// Generate a TEST() for each spec example (1-655)
// Using a macro to avoid writing 655 individual TEST() calls
#define GEN_SPEC_TEST(n)                                              \
  TEST(CommonMarkSpec, Example_##n) {                                 \
    const std::string main_path = GetMainBinaryPath();                \
    const std::string spec_path = GetSpecPath();                      \
    std::string result = RunPythonTest(n, main_path, spec_path);      \
    ASSERT_EQ("PASS", result) << "Test example " << n << " failed:\n" \
                              << result;                              \
  }

// Generate tests for examples 1 through 655
GEN_SPEC_TEST(1)
GEN_SPEC_TEST(2) GEN_SPEC_TEST(3) GEN_SPEC_TEST(4) GEN_SPEC_TEST(5) GEN_SPEC_TEST(
    6) GEN_SPEC_TEST(7) GEN_SPEC_TEST(8) GEN_SPEC_TEST(9) GEN_SPEC_TEST(10)
    GEN_SPEC_TEST(11) GEN_SPEC_TEST(12) GEN_SPEC_TEST(13) GEN_SPEC_TEST(14) GEN_SPEC_TEST(
        15) GEN_SPEC_TEST(16) GEN_SPEC_TEST(17) GEN_SPEC_TEST(18) GEN_SPEC_TEST(19)
        GEN_SPEC_TEST(20) GEN_SPEC_TEST(21) GEN_SPEC_TEST(22) GEN_SPEC_TEST(23) GEN_SPEC_TEST(
            24) GEN_SPEC_TEST(25) GEN_SPEC_TEST(26) GEN_SPEC_TEST(27) GEN_SPEC_TEST(28)
            GEN_SPEC_TEST(29) GEN_SPEC_TEST(30) GEN_SPEC_TEST(31) GEN_SPEC_TEST(
                32) GEN_SPEC_TEST(33) GEN_SPEC_TEST(34) GEN_SPEC_TEST(35)
                GEN_SPEC_TEST(36) GEN_SPEC_TEST(37) GEN_SPEC_TEST(38) GEN_SPEC_TEST(
                    39) GEN_SPEC_TEST(40) GEN_SPEC_TEST(41) GEN_SPEC_TEST(42)
                    GEN_SPEC_TEST(43) GEN_SPEC_TEST(44) GEN_SPEC_TEST(45) GEN_SPEC_TEST(
                        46) GEN_SPEC_TEST(47) GEN_SPEC_TEST(48) GEN_SPEC_TEST(49)
                        GEN_SPEC_TEST(50) GEN_SPEC_TEST(51) GEN_SPEC_TEST(52) GEN_SPEC_TEST(
                            53) GEN_SPEC_TEST(54) GEN_SPEC_TEST(55) GEN_SPEC_TEST(56)
                            GEN_SPEC_TEST(57) GEN_SPEC_TEST(58) GEN_SPEC_TEST(
                                59) GEN_SPEC_TEST(60) GEN_SPEC_TEST(61) GEN_SPEC_TEST(62)
                                GEN_SPEC_TEST(63) GEN_SPEC_TEST(64) GEN_SPEC_TEST(
                                    65) GEN_SPEC_TEST(66) GEN_SPEC_TEST(67)
                                    GEN_SPEC_TEST(68) GEN_SPEC_TEST(69) GEN_SPEC_TEST(
                                        70) GEN_SPEC_TEST(71) GEN_SPEC_TEST(72)
                                        GEN_SPEC_TEST(73) GEN_SPEC_TEST(74) GEN_SPEC_TEST(
                                            75) GEN_SPEC_TEST(76) GEN_SPEC_TEST(77)
                                            GEN_SPEC_TEST(78) GEN_SPEC_TEST(79) GEN_SPEC_TEST(
                                                80) GEN_SPEC_TEST(81) GEN_SPEC_TEST(82)
                                                GEN_SPEC_TEST(83) GEN_SPEC_TEST(
                                                    84) GEN_SPEC_TEST(85) GEN_SPEC_TEST(86)
                                                    GEN_SPEC_TEST(87) GEN_SPEC_TEST(
                                                        88) GEN_SPEC_TEST(89) GEN_SPEC_TEST(90)
                                                        GEN_SPEC_TEST(91) GEN_SPEC_TEST(
                                                            92) GEN_SPEC_TEST(93) GEN_SPEC_TEST(94)
                                                            GEN_SPEC_TEST(95) GEN_SPEC_TEST(
                                                                96) GEN_SPEC_TEST(97)
                                                                GEN_SPEC_TEST(98) GEN_SPEC_TEST(
                                                                    99) GEN_SPEC_TEST(100)
                                                                    GEN_SPEC_TEST(101) GEN_SPEC_TEST(
                                                                        102) GEN_SPEC_TEST(103)
                                                                        GEN_SPEC_TEST(104) GEN_SPEC_TEST(
                                                                            105) GEN_SPEC_TEST(106)
                                                                            GEN_SPEC_TEST(
                                                                                107) GEN_SPEC_TEST(108)
                                                                                GEN_SPEC_TEST(
                                                                                    109)
                                                                                    GEN_SPEC_TEST(
                                                                                        110)
                                                                                        GEN_SPEC_TEST(
                                                                                            111)
                                                                                            GEN_SPEC_TEST(
                                                                                                112)
                                                                                                GEN_SPEC_TEST(
                                                                                                    113)
                                                                                                    GEN_SPEC_TEST(
                                                                                                        114)
                                                                                                        GEN_SPEC_TEST(
                                                                                                            115) GEN_SPEC_TEST(116) GEN_SPEC_TEST(117) GEN_SPEC_TEST(118) GEN_SPEC_TEST(119) GEN_SPEC_TEST(120) GEN_SPEC_TEST(121) GEN_SPEC_TEST(122) GEN_SPEC_TEST(123) GEN_SPEC_TEST(124) GEN_SPEC_TEST(125) GEN_SPEC_TEST(126) GEN_SPEC_TEST(127) GEN_SPEC_TEST(128) GEN_SPEC_TEST(129) GEN_SPEC_TEST(130) GEN_SPEC_TEST(131) GEN_SPEC_TEST(132) GEN_SPEC_TEST(133) GEN_SPEC_TEST(134) GEN_SPEC_TEST(135) GEN_SPEC_TEST(136)
                                                                                                            GEN_SPEC_TEST(
                                                                                                                137)
                                                                                                                GEN_SPEC_TEST(
                                                                                                                    138)
                                                                                                                    GEN_SPEC_TEST(
                                                                                                                        139)
                                                                                                                        GEN_SPEC_TEST(
                                                                                                                            140)
                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                141)
                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                    142) GEN_SPEC_TEST(143) GEN_SPEC_TEST(144) GEN_SPEC_TEST(145) GEN_SPEC_TEST(146) GEN_SPEC_TEST(147) GEN_SPEC_TEST(148) GEN_SPEC_TEST(149)
                                                                                                                                    GEN_SPEC_TEST(150) GEN_SPEC_TEST(
                                                                                                                                        151) GEN_SPEC_TEST(152)
                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                            153)
                                                                                                                                            GEN_SPEC_TEST(154) GEN_SPEC_TEST(155) GEN_SPEC_TEST(156) GEN_SPEC_TEST(157) GEN_SPEC_TEST(158) GEN_SPEC_TEST(159) GEN_SPEC_TEST(
                                                                                                                                                160)
                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                    161)
                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                        162)
                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                            163)
                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                164)
                                                                                                                                                                GEN_SPEC_TEST(165) GEN_SPEC_TEST(166) GEN_SPEC_TEST(167) GEN_SPEC_TEST(168) GEN_SPEC_TEST(169) GEN_SPEC_TEST(
                                                                                                                                                                    170) GEN_SPEC_TEST(171) GEN_SPEC_TEST(172) GEN_SPEC_TEST(173) GEN_SPEC_TEST(174) GEN_SPEC_TEST(175) GEN_SPEC_TEST(176)
                                                                                                                                                                    GEN_SPEC_TEST(177) GEN_SPEC_TEST(178) GEN_SPEC_TEST(
                                                                                                                                                                        179) GEN_SPEC_TEST(180) GEN_SPEC_TEST(181) GEN_SPEC_TEST(182) GEN_SPEC_TEST(183) GEN_SPEC_TEST(184) GEN_SPEC_TEST(185) GEN_SPEC_TEST(186) GEN_SPEC_TEST(187) GEN_SPEC_TEST(188) GEN_SPEC_TEST(189)
                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                            190) GEN_SPEC_TEST(191)
                                                                                                                                                                            GEN_SPEC_TEST(192) GEN_SPEC_TEST(193) GEN_SPEC_TEST(194) GEN_SPEC_TEST(195) GEN_SPEC_TEST(196) GEN_SPEC_TEST(197) GEN_SPEC_TEST(198) GEN_SPEC_TEST(199) GEN_SPEC_TEST(200) GEN_SPEC_TEST(201) GEN_SPEC_TEST(202) GEN_SPEC_TEST(203) GEN_SPEC_TEST(204) GEN_SPEC_TEST(205) GEN_SPEC_TEST(206) GEN_SPEC_TEST(207) GEN_SPEC_TEST(208) GEN_SPEC_TEST(209) GEN_SPEC_TEST(210) GEN_SPEC_TEST(211) GEN_SPEC_TEST(212) GEN_SPEC_TEST(213) GEN_SPEC_TEST(214) GEN_SPEC_TEST(215) GEN_SPEC_TEST(216) GEN_SPEC_TEST(217) GEN_SPEC_TEST(218) GEN_SPEC_TEST(219) GEN_SPEC_TEST(220) GEN_SPEC_TEST(221) GEN_SPEC_TEST(222) GEN_SPEC_TEST(223) GEN_SPEC_TEST(224) GEN_SPEC_TEST(225) GEN_SPEC_TEST(
                                                                                                                                                                                226) GEN_SPEC_TEST(227) GEN_SPEC_TEST(228) GEN_SPEC_TEST(229) GEN_SPEC_TEST(230) GEN_SPEC_TEST(231) GEN_SPEC_TEST(232) GEN_SPEC_TEST(233) GEN_SPEC_TEST(234) GEN_SPEC_TEST(235) GEN_SPEC_TEST(236) GEN_SPEC_TEST(237)
                                                                                                                                                                                GEN_SPEC_TEST(238) GEN_SPEC_TEST(239) GEN_SPEC_TEST(240) GEN_SPEC_TEST(241) GEN_SPEC_TEST(242) GEN_SPEC_TEST(243) GEN_SPEC_TEST(244) GEN_SPEC_TEST(245) GEN_SPEC_TEST(246) GEN_SPEC_TEST(
                                                                                                                                                                                    247) GEN_SPEC_TEST(248)
                                                                                                                                                                                    GEN_SPEC_TEST(249) GEN_SPEC_TEST(
                                                                                                                                                                                        250) GEN_SPEC_TEST(251) GEN_SPEC_TEST(252) GEN_SPEC_TEST(253) GEN_SPEC_TEST(254) GEN_SPEC_TEST(255) GEN_SPEC_TEST(256) GEN_SPEC_TEST(257) GEN_SPEC_TEST(258) GEN_SPEC_TEST(259)
                                                                                                                                                                                        GEN_SPEC_TEST(260)
                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                261)
                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                    262)
                                                                                                                                                                                                    GEN_SPEC_TEST(263) GEN_SPEC_TEST(264) GEN_SPEC_TEST(265) GEN_SPEC_TEST(266) GEN_SPEC_TEST(
                                                                                                                                                                                                        267)
                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                            268)
                                                                                                                                                                                                            GEN_SPEC_TEST(269) GEN_SPEC_TEST(270) GEN_SPEC_TEST(271) GEN_SPEC_TEST(272) GEN_SPEC_TEST(273)
                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                    274)
                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                        275)
                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                            276) GEN_SPEC_TEST(277) GEN_SPEC_TEST(278) GEN_SPEC_TEST(279)
                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                280)
                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                    281)
                                                                                                                                                                                                                                    GEN_SPEC_TEST(282) GEN_SPEC_TEST(283) GEN_SPEC_TEST(284) GEN_SPEC_TEST(285) GEN_SPEC_TEST(286) GEN_SPEC_TEST(
                                                                                                                                                                                                                                        287) GEN_SPEC_TEST(288) GEN_SPEC_TEST(289) GEN_SPEC_TEST(290) GEN_SPEC_TEST(291)
                                                                                                                                                                                                                                        GEN_SPEC_TEST(292) GEN_SPEC_TEST(293) GEN_SPEC_TEST(294) GEN_SPEC_TEST(295) GEN_SPEC_TEST(
                                                                                                                                                                                                                                            296)
                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                297)
                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                    298)
                                                                                                                                                                                                                                                    GEN_SPEC_TEST(299) GEN_SPEC_TEST(300) GEN_SPEC_TEST(301) GEN_SPEC_TEST(302) GEN_SPEC_TEST(303) GEN_SPEC_TEST(304) GEN_SPEC_TEST(305) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                        306)
                                                                                                                                                                                                                                                        GEN_SPEC_TEST(307) GEN_SPEC_TEST(308) GEN_SPEC_TEST(309) GEN_SPEC_TEST(310) GEN_SPEC_TEST(311) GEN_SPEC_TEST(312) GEN_SPEC_TEST(313) GEN_SPEC_TEST(314) GEN_SPEC_TEST(315) GEN_SPEC_TEST(316) GEN_SPEC_TEST(317)
                                                                                                                                                                                                                                                            GEN_SPEC_TEST(318) GEN_SPEC_TEST(319) GEN_SPEC_TEST(320) GEN_SPEC_TEST(321) GEN_SPEC_TEST(322)
                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                    323) GEN_SPEC_TEST(324)
                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                        325) GEN_SPEC_TEST(326) GEN_SPEC_TEST(327)
                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                            328) GEN_SPEC_TEST(329) GEN_SPEC_TEST(330) GEN_SPEC_TEST(331) GEN_SPEC_TEST(332)
                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                333)
                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(334) GEN_SPEC_TEST(335) GEN_SPEC_TEST(336) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                    337) GEN_SPEC_TEST(338)
                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(339) GEN_SPEC_TEST(340) GEN_SPEC_TEST(341) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                        342)
                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                            343)
                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                344) GEN_SPEC_TEST(345) GEN_SPEC_TEST(346)
                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(347) GEN_SPEC_TEST(348) GEN_SPEC_TEST(349) GEN_SPEC_TEST(350) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                    351)
                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                        352)
                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                            353) GEN_SPEC_TEST(354) GEN_SPEC_TEST(355)
                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                356)
                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                    357)
                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                        358)
                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                            359) GEN_SPEC_TEST(360) GEN_SPEC_TEST(361) GEN_SPEC_TEST(362) GEN_SPEC_TEST(363)
                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                364)
                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(365) GEN_SPEC_TEST(366) GEN_SPEC_TEST(367)
                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(368) GEN_SPEC_TEST(369) GEN_SPEC_TEST(370) GEN_SPEC_TEST(371)
                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                            372)
                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                373)
                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                    374) GEN_SPEC_TEST(375)
                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                        376)
                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(377) GEN_SPEC_TEST(378) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                            379) GEN_SPEC_TEST(380)
                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                381) GEN_SPEC_TEST(382) GEN_SPEC_TEST(383)
                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(384) GEN_SPEC_TEST(385) GEN_SPEC_TEST(386) GEN_SPEC_TEST(387) GEN_SPEC_TEST(388) GEN_SPEC_TEST(389)
                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                        390)
                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(391) GEN_SPEC_TEST(392) GEN_SPEC_TEST(393) GEN_SPEC_TEST(394) GEN_SPEC_TEST(395) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                            396)
                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                397)
                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                    398) GEN_SPEC_TEST(399)
                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                        400)
                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                            401)
                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                402)
                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                    403)
                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(404) GEN_SPEC_TEST(405) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                        406) GEN_SPEC_TEST(407)
                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                            408) GEN_SPEC_TEST(409)
                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                410)
                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                    411)
                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                        412)
                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(413) GEN_SPEC_TEST(414) GEN_SPEC_TEST(415)
                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(416) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                417) GEN_SPEC_TEST(418)
                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                    419)
                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(420) GEN_SPEC_TEST(421)
                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                            422)
                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                423)
                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                    424) GEN_SPEC_TEST(425) GEN_SPEC_TEST(426) GEN_SPEC_TEST(427)
                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(428) GEN_SPEC_TEST(429) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                        430) GEN_SPEC_TEST(431) GEN_SPEC_TEST(432) GEN_SPEC_TEST(433) GEN_SPEC_TEST(434) GEN_SPEC_TEST(435) GEN_SPEC_TEST(436) GEN_SPEC_TEST(437) GEN_SPEC_TEST(438) GEN_SPEC_TEST(439) GEN_SPEC_TEST(440) GEN_SPEC_TEST(441) GEN_SPEC_TEST(442) GEN_SPEC_TEST(443) GEN_SPEC_TEST(444)
                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(445) GEN_SPEC_TEST(446) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                            447) GEN_SPEC_TEST(448)
                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(449) GEN_SPEC_TEST(450) GEN_SPEC_TEST(451) GEN_SPEC_TEST(452) GEN_SPEC_TEST(453) GEN_SPEC_TEST(454) GEN_SPEC_TEST(455) GEN_SPEC_TEST(456) GEN_SPEC_TEST(457) GEN_SPEC_TEST(458) GEN_SPEC_TEST(459) GEN_SPEC_TEST(460) GEN_SPEC_TEST(461) GEN_SPEC_TEST(462) GEN_SPEC_TEST(463) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                464) GEN_SPEC_TEST(465)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                    466) GEN_SPEC_TEST(467) GEN_SPEC_TEST(468) GEN_SPEC_TEST(469) GEN_SPEC_TEST(470) GEN_SPEC_TEST(471) GEN_SPEC_TEST(472)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                        473) GEN_SPEC_TEST(474)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(475) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                            476)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                477) GEN_SPEC_TEST(478) GEN_SPEC_TEST(479) GEN_SPEC_TEST(480) GEN_SPEC_TEST(481) GEN_SPEC_TEST(482) GEN_SPEC_TEST(483)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    484) GEN_SPEC_TEST(485) GEN_SPEC_TEST(486) GEN_SPEC_TEST(487) GEN_SPEC_TEST(488) GEN_SPEC_TEST(489) GEN_SPEC_TEST(490) GEN_SPEC_TEST(491) GEN_SPEC_TEST(492) GEN_SPEC_TEST(493) GEN_SPEC_TEST(494) GEN_SPEC_TEST(495) GEN_SPEC_TEST(496) GEN_SPEC_TEST(497)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(498) GEN_SPEC_TEST(499) GEN_SPEC_TEST(500) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        501) GEN_SPEC_TEST(502) GEN_SPEC_TEST(503) GEN_SPEC_TEST(504)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(505) GEN_SPEC_TEST(506) GEN_SPEC_TEST(507) GEN_SPEC_TEST(508) GEN_SPEC_TEST(509) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            510) GEN_SPEC_TEST(511) GEN_SPEC_TEST(512)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(513) GEN_SPEC_TEST(514) GEN_SPEC_TEST(515) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                516) GEN_SPEC_TEST(517)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(518) GEN_SPEC_TEST(519) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    520) GEN_SPEC_TEST(521) GEN_SPEC_TEST(522)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(523) GEN_SPEC_TEST(524) GEN_SPEC_TEST(525) GEN_SPEC_TEST(526) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        527) GEN_SPEC_TEST(528) GEN_SPEC_TEST(529) GEN_SPEC_TEST(530) GEN_SPEC_TEST(531)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(532) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            533) GEN_SPEC_TEST(534)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(535) GEN_SPEC_TEST(536) GEN_SPEC_TEST(537) GEN_SPEC_TEST(538) GEN_SPEC_TEST(539) GEN_SPEC_TEST(540) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                541)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(542) GEN_SPEC_TEST(543) GEN_SPEC_TEST(544) GEN_SPEC_TEST(545) GEN_SPEC_TEST(546) GEN_SPEC_TEST(547) GEN_SPEC_TEST(548) GEN_SPEC_TEST(549) GEN_SPEC_TEST(550) GEN_SPEC_TEST(551) GEN_SPEC_TEST(552) GEN_SPEC_TEST(553) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    554)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(555)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            556)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(557) GEN_SPEC_TEST(558) GEN_SPEC_TEST(559) GEN_SPEC_TEST(560) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                561) GEN_SPEC_TEST(562) GEN_SPEC_TEST(563) GEN_SPEC_TEST(564) GEN_SPEC_TEST(565)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(566) GEN_SPEC_TEST(567) GEN_SPEC_TEST(568) GEN_SPEC_TEST(569) GEN_SPEC_TEST(570) GEN_SPEC_TEST(571) GEN_SPEC_TEST(572) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    573) GEN_SPEC_TEST(574) GEN_SPEC_TEST(575) GEN_SPEC_TEST(576)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(577) GEN_SPEC_TEST(578) GEN_SPEC_TEST(579) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        580) GEN_SPEC_TEST(581) GEN_SPEC_TEST(582) GEN_SPEC_TEST(583)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(584) GEN_SPEC_TEST(585) GEN_SPEC_TEST(586) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            587) GEN_SPEC_TEST(588) GEN_SPEC_TEST(589) GEN_SPEC_TEST(590)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(591) GEN_SPEC_TEST(592) GEN_SPEC_TEST(593) GEN_SPEC_TEST(594) GEN_SPEC_TEST(595) GEN_SPEC_TEST(596) GEN_SPEC_TEST(597) GEN_SPEC_TEST(598) GEN_SPEC_TEST(599) GEN_SPEC_TEST(600) GEN_SPEC_TEST(601) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                602) GEN_SPEC_TEST(603) GEN_SPEC_TEST(604)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(605) GEN_SPEC_TEST(606) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    607) GEN_SPEC_TEST(608) GEN_SPEC_TEST(609)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(610) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        611) GEN_SPEC_TEST(612) GEN_SPEC_TEST(613)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(614) GEN_SPEC_TEST(615) GEN_SPEC_TEST(616) GEN_SPEC_TEST(617) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            618) GEN_SPEC_TEST(619) GEN_SPEC_TEST(620) GEN_SPEC_TEST(621) GEN_SPEC_TEST(622) GEN_SPEC_TEST(623) GEN_SPEC_TEST(624) GEN_SPEC_TEST(625) GEN_SPEC_TEST(626) GEN_SPEC_TEST(627)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                628) GEN_SPEC_TEST(629) GEN_SPEC_TEST(630) GEN_SPEC_TEST(631) GEN_SPEC_TEST(632) GEN_SPEC_TEST(633) GEN_SPEC_TEST(634) GEN_SPEC_TEST(635) GEN_SPEC_TEST(636)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    637)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        638)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        GEN_SPEC_TEST(639) GEN_SPEC_TEST(640) GEN_SPEC_TEST(641) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            642)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                643) GEN_SPEC_TEST(644) GEN_SPEC_TEST(645) GEN_SPEC_TEST(646) GEN_SPEC_TEST(647) GEN_SPEC_TEST(648) GEN_SPEC_TEST(649) GEN_SPEC_TEST(650)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                GEN_SPEC_TEST(651) GEN_SPEC_TEST(652) GEN_SPEC_TEST(653) GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    654)
GEN_SPEC_TEST(
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     655)

// =============================================================================
// Streaming API tests
// =============================================================================

namespace {

std::string ToStd(const std::pmr::string& s) {
  return std::string(s.data(), s.size());
}

std::string Regular(const std::string& input,
                    const markus::Options& options = {}) {
  return ToStd(markus::MarkdownToHtml(input, options));
}

// Feed the whole input at once, then flush; concatenate every emission.
std::string StreamSingleFeed(const std::string& input,
                             const markus::Options& options = {}) {
  std::string out;
  markus::StreamingMarkdownParser parser(options);
  parser.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });
  parser.Feed(input);
  parser.Flush();
  return out;
}

// Feed `input` in `chunk`-byte pieces, then flush; concatenate emissions.
std::string StreamChunked(const std::string& input, size_t chunk,
                          const markus::Options& options = {}) {
  std::string out;
  markus::StreamingMarkdownParser parser(options);
  parser.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });
  for (size_t i = 0; i < input.size(); i += chunk) {
    size_t len = std::min(chunk, input.size() - i);
    parser.Feed(std::string_view(input.data() + i, len));
  }
  parser.Flush();
  return out;
}

// Collect each individual emission (rather than concatenating).
std::vector<std::string> StreamEmissions(const std::string& input, size_t chunk,
                                         const markus::Options& options = {}) {
  std::vector<std::string> emissions;
  markus::StreamingMarkdownParser parser(options);
  parser.setOutputCallback([&](std::string_view h) {
    emissions.emplace_back(h.data(), h.size());
  });
  for (size_t i = 0; i < input.size(); i += chunk) {
    size_t len = std::min(chunk, input.size() - i);
    parser.Feed(std::string_view(input.data() + i, len));
  }
  parser.Flush();
  return emissions;
}

}  // namespace

// The strong guarantee: feeding the entire document in one chunk and flushing
// reproduces MarkdownToHtml exactly.
TEST(StreamingMarkdownParser, SingleFeedMatchesRegular) {
  const std::vector<std::string> inputs = {
      "hello\n",
      "para one\n\npara two\n",
      "# heading\n",
      "- a\n- b\n- c\n",
      "1. one\n2. two\n",
      "> quoted\nline\n",
      "```\ncode\n```\n",
      "    indented\n    code\n",
      "<style>x</style>\nafter\n",
      "a\n\n---\n\nb\n",
      "text **bold** _em_ `code`\n",
      "[link](http://x.com)\n",
      "![img](y.png)\n",
      "<style\n  type=\"text/css\">\n\nfoo\n",  // spec Example 173
      "para1\n\n<style\nfoo\n",
      "para1\n\n<style>foo</style>\npara2\n",
      "[ref]: /url\n\n[ref]\n",
  };
  for (const auto& in : inputs) {
    EXPECT_EQ(Regular(in), StreamSingleFeed(in)) << "input: " << in;
  }
}

// Chunked (and char-by-char) feeding matches the regular output for common,
// well-behaved inputs that have no cross-blank-line ambiguity.
TEST(StreamingMarkdownParser, ChunkedMatchesRegular) {
  const std::vector<std::string> inputs = {
      "para one\n\npara two\n",
      "# heading\n",
      "- a\n- b\n- c\n",
      "1. one\n2. two\n",
      "> quoted\nline\n",
      "```\ncode\n```\n",
      "    indented\n    code\n",
      "<style>x</style>\nafter\n",
      "a\n\n---\n\nb\n",
      "text **bold** _em_ `code`\n",
      "[ref]: /url\n\n[ref]\n",
      "<style\n  type=\"text/css\">\n\nfoo\n",  // spec Example 173
  };
  for (const auto& in : inputs) {
    for (size_t chunk : {1u, 2u, 3u, 5u, 8u, 64u}) {
      EXPECT_EQ(Regular(in), StreamChunked(in, chunk))
          << "input: " << in << " chunk: " << chunk;
    }
  }
}

// An unterminated raw-HTML block must be held back: it is emitted neither
// during Feed (while the block is still open) nor split across a mid-block
// boundary, and the whole block is only emitted once it is closed or on Flush.
TEST(StreamingMarkdownParser, UnterminatedRawHtmlIsHeldBack) {
  // Example 173: `<style` with no closing tag absorbs the blank line and `foo`.
  const std::string input = "<style\n  type=\"text/css\">\n\nfoo\n";

  // Nothing may be emitted until the block is terminated or flushed.
  markus::StreamingMarkdownParser parser;
  std::string emitted_so_far;
  parser.setOutputCallback([&](std::string_view h) {
    emitted_so_far.append(h.data(), h.size());
  });
  parser.Feed(input);
  EXPECT_EQ(std::string(), emitted_so_far)
      << "open raw-HTML block must not be emitted before it is closed";
  parser.Flush();
  EXPECT_EQ(Regular(input), emitted_so_far);

  // A type-6 block (no explicit end tag) is likewise held until a blank line or
  // EOF terminates it.
  EXPECT_EQ(Regular("<div>\nfoo\n"), StreamChunked("<div>\nfoo\n", 2));
  EXPECT_EQ(Regular("<div>\nfoo\n\nbar\n"),
            StreamChunked("<div>\nfoo\n\nbar\n", 2));
}

// Emissions arrive in document order, each is a prefix-continuation of the
// output, and the trailing incomplete block is only emitted on Flush.
TEST(StreamingMarkdownParser, ProgressiveEmissionOrder) {
  const std::string input = "para1\n\npara2\n\npara3\n";
  const std::string expected = Regular(input);

  std::string running;
  bool saw_trailing_held = false;
  {
    markus::StreamingMarkdownParser parser;
    parser.setOutputCallback([&](std::string_view h) {
      running.append(h.data(), h.size());
      // Every prefix of the emitted output must itself be a prefix of the final
      // document (blocks are never revised or reordered).
      EXPECT_EQ(0u, expected.rfind(running, 0) == 0 ? 0u : 1u)
          << "emissions must stay a prefix of the final output; running: "
          << running;
    });
    // Feed block-aligned so the first two paragraphs complete mid-stream.
    parser.Feed("para1\n\n");
    parser.Feed("para2\n\n");
    // The trailing paragraph is still open: it must not be emitted yet.
    parser.Feed("para3\n");
    saw_trailing_held = (running == Regular("para1\n\npara2\n\n"));
    parser.Flush();
  }
  EXPECT_TRUE(saw_trailing_held)
      << "the trailing (incomplete) paragraph must be held until Flush; got: "
      << running;
  EXPECT_EQ(expected, running);
}

// GFM extensions stream identically to the regular (non-streaming) renderer.
TEST(StreamingMarkdownParser, GfmOptionsMatchRegular) {
  struct Case {
    markus::Options options;
    std::string input;
  };
  const std::vector<Case> cases = {
      {markus::Options{.enable_tables = true},
       "| a | b |\n|---|---|\n| 1 | 2 |\n"},
      {markus::Options{.enable_strikethrough = true}, "~~gone~~\n"},
      {markus::Options{.enable_tasklist = true}, "- [x] done\n- [ ] todo\n"},
      {markus::Options{.enable_autolink = true},
       "visit https://example.com now\n"},
      {markus::Options{.enable_tables = true, .enable_tasklist = true},
       "- [ ] a\n\n| x |\n|---|\n| y |\n"},
  };
  for (const auto& c : cases) {
    EXPECT_EQ(Regular(c.input, c.options), StreamSingleFeed(c.input, c.options))
        << "single-feed mismatch for GFM input: " << c.input;
  }
}

// A link reference defined before its use resolves even when they arrive in
// separate chunks.
TEST(StreamingMarkdownParser, LinkRefDefinedBeforeUseResolves) {
  const std::string input = "[ref]: /url\n\n[ref]\n";
  // The definition line settles (and is remembered) before the use is emitted.
  EXPECT_EQ(Regular(input), StreamChunked(input, 2));
  EXPECT_EQ(Regular(input), StreamSingleFeed(input));
}

// Inherent one-pass limitation: a reference link used before its definition
// arrives cannot be retroactively turned into a link, because the earlier
// paragraph has already been emitted. The definition is still remembered for
// any *later* use.
TEST(StreamingMarkdownParser, ForwardRefLimitDocumented) {
  const std::string input = "[ref]\n\n[ref]: /url\n";
  // Use first, then definition. The emitted paragraph keeps the literal text.
  EXPECT_EQ(std::string("<p>[ref]</p>\n"),
            StreamChunked(input, input.find_first_of('\n') + 1));
  // By contrast, the full document (single feed) resolves it.
  EXPECT_EQ(Regular(input), StreamSingleFeed(input));
}

// Empty Feed() is a no-op; Reset() discards all buffered content and link
// references, behaving like a fresh parser.
TEST(StreamingMarkdownParser, ResetAndEmptyFeed) {
  markus::StreamingMarkdownParser parser;
  std::string out;
  parser.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });

  parser.Feed("");  // no-op
  EXPECT_TRUE(parser.empty());
  parser.Flush();
  EXPECT_EQ(std::string(), out);

  parser.Feed("para1\n");
  parser.Reset();
  EXPECT_TRUE(parser.empty());
  parser.Feed("para2\n");
  parser.Flush();
  EXPECT_EQ(Regular("para2\n"), out)
      << "content before Reset() must be discarded; got: " << out;
}

// A line without a trailing newline cannot be completed; it is held together
// with subsequent input until a newline arrives, then (if still open) until
// Flush.
TEST(StreamingMarkdownParser, PartialLineHeldUntilNewline) {
  markus::StreamingMarkdownParser parser;
  std::string out;
  parser.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });
  parser.Feed("para");
  EXPECT_TRUE(parser.empty() == false);
  EXPECT_EQ(std::string(), out);  // no newline yet: nothing can be emitted
  parser.Feed(" more\n");
  parser.Flush();
  EXPECT_EQ(Regular("para more\n"), out);
}

// A block quote interrupted by a blank (empty) quote line is still the same
// block: a further `> ` line re-joins it rather than starting a second
// blockquote. This is the correctness fix over the old sentinel probe.
TEST(StreamingMarkdownParser, QuoteRejoinsAfterBlankLine) {
  const std::string whole = "> a\n>\n> more\n";
  std::string out;
  markus::StreamingMarkdownParser parser;
  parser.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });
  parser.Feed("> a\n");
  parser.Feed(">\n");
  parser.Feed("> more\n");
  parser.Flush();
  EXPECT_EQ(Regular(whole), out)
      << "an open block quote must re-join across the blank quote line, not "
      << "split into two; got: " << out;
}

// Same for lists: a further item after the separating blank line extends the
// same (loose) list instead of opening a new one.
TEST(StreamingMarkdownParser, ListRejoinsAfterBlankLine) {
  const std::string whole = "1. a\n\n2. b\n";
  std::string out;
  markus::StreamingMarkdownParser parser;
  parser.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });
  parser.Feed("1. a\n\n");
  parser.Feed("2. b\n");
  parser.Flush();
  EXPECT_EQ(Regular(whole), out)
      << "an open list must re-join across the blank line (one loose <ol>), "
      << "not two lists; got: " << out;
}

// The held-back buffer is capped. A chunk that would push it past the limit
// throws std::length_error and leaves the buffer unchanged; a large enough
// limit handles the same input fine.
TEST(StreamingMarkdownParser, PendingLimit) {
  std::string chunk(19, 'a');
  chunk.push_back('\n');  // 20 bytes, contains a newline

  markus::StreamingMarkdownParser small;
  small.setPendingLimit(16);
  EXPECT_THROW(small.Feed(chunk), std::length_error);
  EXPECT_TRUE(small.empty());  // buffer unchanged (nothing appended before throw)

  markus::StreamingMarkdownParser big;
  std::string out;
  big.setOutputCallback(
      [&](std::string_view h) { out.append(h.data(), h.size()); });
  big.setPendingLimit(1024);
  big.Feed(chunk);
  big.Flush();
  EXPECT_EQ(Regular(chunk), out);
}

// A large (>= 256 KB) mixed document streamed in small chunks reproduces the
// regular output exactly, at several chunk sizes.
TEST(StreamingMarkdownParser, LargeChunkedMatchesRegular) {
  std::string input;
  input.reserve(280 * 1024);
  for (int i = 0; input.size() < 256 * 1024; ++i) {
    switch (i % 6) {
      case 0:
        input += "# Heading " + std::to_string(i) + "\n";
        break;
      case 1:
        input +=
            "A paragraph with **bold** and `code` number " + std::to_string(i) +
            ".\n\n";
        break;
      case 2:
        input += "- item one\n- item two " + std::to_string(i) + "\n\n";
        break;
      case 3:
        input += "```\ncode line " + std::to_string(i) + "\n```\n";
        break;
      case 4:
        input += "> quote line " + std::to_string(i) + "\n>\n> more\n\n";
        break;
      case 5:
        input += "1. first\n2. second " + std::to_string(i) + "\n\n";
        break;
    }
  }
  for (size_t chunk : {3u, 1024u, 8192u}) {
    EXPECT_EQ(Regular(input), StreamChunked(input, chunk))
        << "chunk: " << chunk << " size: " << input.size();
  }
}

// A single large (>= 256 KB) fenced code block streamed in small chunks
// completes on Flush with output identical to the regular render. The block
// stays open (held back) until its closing fence, so this guards the cost of
// re-rendering a large open trailing block.
TEST(StreamingMarkdownParser, StreamedLargeOpenBlock) {
  std::string input = "```\n";
  while (input.size() < 260 * 1024) {
    input += "line of code that is reasonably long\n";
  }
  input += "```\n";
  for (size_t chunk : {257u, 4096u}) {
    EXPECT_EQ(Regular(input), StreamChunked(input, chunk))
        << "chunk: " << chunk << " size: " << input.size();
  }
}

// Out-of-range / overflowing numeric character references must produce
// deterministic output (the ParseUint overflow clamp to UINT32_MAX), never an
// undefined wrapped code point. The inline path rejects the out-of-range
// reference and renders it literally; the fenced-code info-string path clamps
// to U+FFFD.
TEST(Markus, ParseUintOverflow) {
  EXPECT_EQ(std::string("<p>&amp;#99999999999999;</p>\n"),
            Regular("&#99999999999999;"));
  const std::string hex40(40, 'f');
  EXPECT_EQ(std::string("<p>&amp;#x") + hex40 + ";</p>\n",
            Regular("&#x" + hex40 + ";"));
  const std::string info_expected =
      "<pre><code class=\"language-\xEF\xBF\xBD\">code\n</code></pre>\n";
  EXPECT_EQ(info_expected, Regular("```&#99999999999999;\ncode\n```\n"));
}

// Repeated parse+render of the same document is byte-for-byte stable across
// many iterations (functional guard; memory flatness is verified separately).
TEST(Markus, RepeatedParseStable) {
  std::string doc;
  doc.reserve(60 * 1024);
  for (int i = 0; i < 400; ++i) {
    doc += "## Section " + std::to_string(i) + "\n\n";
    doc += "Text with **bold**, *em* and `code` plus a [link](http://x.com/i) " +
           std::to_string(i) + ".\n\n";
    doc += "```\ncode " + std::to_string(i) + "\n```\n\n";
  }
  const std::string first = Regular(doc);
  for (int i = 0; i < 200; ++i) {
    EXPECT_EQ(first, Regular(doc)) << "iteration " << i << " size " << doc.size();
  }
}

// =============================================================================
// DetailsBlock (GitHub-style <details> sections)
// =============================================================================

const markus::DetailsBlock* GetDetails(const markus::Document& doc, size_t idx) {
  if (idx >= doc.children.size()) return nullptr;
  return std::get_if<markus::DetailsBlock>(&doc.children[idx]);
}

TEST(DetailsBlock, ParsesSummaryAndContent) {
  const std::string input = "<details>\n<summary>Analysis</summary>\n"
                            "body line 1\nbody line 2\n</details>\n";
  markus::Document doc = markus::Parse(input);
  ASSERT_EQ(1u, doc.children.size());
  const markus::DetailsBlock* d = GetDetails(doc, 0);
  ASSERT_NE(nullptr, d);
  EXPECT_TRUE(d->closed);
  EXPECT_EQ("Analysis", ToStd(d->summary));
  EXPECT_NE(std::string::npos, ToStd(d->content).find("body line 1\nbody line 2"));
}

TEST(DetailsBlock, NoSummary) {
  markus::Document doc = markus::Parse("<details>body text</details>\n");
  const markus::DetailsBlock* d = GetDetails(doc, 0);
  ASSERT_NE(nullptr, d);
  EXPECT_TRUE(d->closed);
  EXPECT_EQ("", ToStd(d->summary));
  EXPECT_EQ("body text", ToStd(d->content));
}

TEST(DetailsBlock, UnclosedAtEofIsOpen) {
  const std::string input = "<details>\n<summary>s</summary>\nbody\n";
  markus::Document doc = markus::Parse(input);
  const markus::DetailsBlock* d = GetDetails(doc, 0);
  ASSERT_NE(nullptr, d);
  EXPECT_FALSE(d->closed);
  // DebugAst reports the open state.
  EXPECT_NE(std::string::npos, ToStd(markus::DebugAst(doc)).find("open"));
}

TEST(DetailsBlock, BlankLinesDoNotTerminate) {
  // Unlike a plain type-6 HTML block (which ends at a blank line), a
  // <details> section consumes through blank lines until </details>.
  const std::string input = "<details>\n<summary>s</summary>\n\nbody\n"
                            "</details>\n\nafter\n";
  markus::Document doc = markus::Parse(input);
  ASSERT_EQ(2u, doc.children.size());
  const markus::DetailsBlock* d = GetDetails(doc, 0);
  ASSERT_NE(nullptr, d);
  EXPECT_TRUE(d->closed);
  EXPECT_NE(std::string::npos, ToStd(d->content).find("body"));
  EXPECT_NE(nullptr, std::get_if<markus::Paragraph>(&doc.children[1]));
}

TEST(DetailsBlock, CaseInsensitiveTags) {
  markus::Document doc =
      markus::Parse("<DeTaIlS><SUMMARY>x</SUMMARY>y</dEtAiLs>\n");
  const markus::DetailsBlock* d = GetDetails(doc, 0);
  ASSERT_NE(nullptr, d);
  EXPECT_TRUE(d->closed);
  EXPECT_EQ("x", ToStd(d->summary));
  EXPECT_EQ("y", ToStd(d->content));
}

TEST(DetailsBlock, RendersHtml) {
  const std::string input = "<details>\n<summary>Analysis</summary>\n"
                            "body\n</details>\n";
  const std::string html = Regular(input);
  EXPECT_NE(std::string::npos, html.find("<details><summary>Analysis</summary>"));
  EXPECT_NE(std::string::npos, html.find("body</details>"));
}

TEST(DetailsBlock, HtmlIsEscaped) {
  const std::string input =
      "<details><summary><b>bold</b></summary>&lt;tag&gt;</details>\n";
  const std::string html = Regular(input);
  // The structured renderer re-escapes its raw text: no live <b> in output.
  EXPECT_EQ(std::string::npos, html.find("<summary><b>"));
  EXPECT_NE(std::string::npos, html.find("&lt;b&gt;"));
}

// A <details> tag that is not a line-leading type-6 open tag does not start
// a section: mid-line tags stay inline HTML in a paragraph, and other tag
// names (e.g. <detailsx>) stay plain HTML blocks.
TEST(DetailsBlock, NotATagStaysHtml) {
  {
    markus::Document doc = markus::Parse("x <details> y\n\npara\n");
    ASSERT_EQ(2u, doc.children.size());
    EXPECT_NE(nullptr, std::get_if<markus::Paragraph>(&doc.children[0]));
  }
  {
    markus::Document doc = markus::Parse("<detailsx>\n\npara\n");
    ASSERT_EQ(2u, doc.children.size());
    EXPECT_NE(nullptr, std::get_if<markus::HtmlBlock>(&doc.children[0]));
  }
}

// =============================================================================
// CodeBlock::fence_char
// =============================================================================

TEST(CodeBlock, FenceChar) {
  {
    markus::Document doc = markus::Parse("```\ncode\n```\n");
    const auto& c = std::get<markus::CodeBlock>(doc.children[0]);
    EXPECT_TRUE(c.is_fenced);
    EXPECT_EQ('`', c.fence_char);
  }
  {
    markus::Document doc = markus::Parse("~~~\ncode\n~~~\n");
    const auto& c = std::get<markus::CodeBlock>(doc.children[0]);
    EXPECT_TRUE(c.is_fenced);
    EXPECT_EQ('~', c.fence_char);
  }
  {
    markus::Document doc = markus::Parse("    indented\n    code\n");
    const auto& c = std::get<markus::CodeBlock>(doc.children[0]);
    EXPECT_FALSE(c.is_fenced);
    EXPECT_EQ(0, c.fence_char);
  }
}

// =============================================================================
// StreamingBlockParser (AST streaming API)
// =============================================================================

// Render only the top-level block range [first, last) of a Document.
std::string RenderRange(const markus::Document& doc, size_t first, size_t last) {
  markus::Document sub = doc;
  sub.children.assign(doc.children.begin() + first, doc.children.begin() + last);
  return ToStd(markus::RenderHtml(sub));
}

std::string BlockStreamSingleFeed(const std::string& input,
                                  const markus::Options& options = {}) {
  std::string out;
  markus::StreamingBlockParser parser(options);
  parser.setBlockCallback([&](const markus::Document& doc, size_t first,
                              size_t last) { out += RenderRange(doc, first, last); });
  parser.Feed(input);
  parser.Flush();
  return out;
}

std::string BlockStreamChunked(const std::string& input, size_t chunk,
                               const markus::Options& options = {}) {
  std::string out;
  markus::StreamingBlockParser parser(options);
  parser.setBlockCallback([&](const markus::Document& doc, size_t first,
                              size_t last) { out += RenderRange(doc, first, last); });
  for (size_t i = 0; i < input.size(); i += chunk) {
    size_t len = std::min(chunk, input.size() - i);
    parser.Feed(std::string_view(input.data() + i, len));
  }
  parser.Flush();
  return out;
}

TEST(StreamingBlockParser, SingleFeedMatchesRegular) {
  const std::vector<std::string> inputs = {
      "hello\n",
      "para one\n\npara two\n",
      "# heading\n",
      "- a\n- b\n- c\n",
      "1. one\n2. two\n",
      "> quoted\nline\n",
      "```\ncode\n```\n",
      "    indented\n    code\n",
      "a\n\n---\n\nb\n",
      "text **bold** _em_ `code`\n",
      "[link](http://x.com)\n",
      "[ref]: /url\n\n[ref]\n",
      "<details>\n<summary>s</summary>\nbody\n</details>\n\nafter\n",
  };
  for (const auto& in : inputs) {
    EXPECT_EQ(Regular(in), BlockStreamSingleFeed(in)) << "input: " << in;
  }
}

TEST(StreamingBlockParser, ChunkedMatchesRegular) {
  const std::vector<std::string> inputs = {
      "para one\n\npara two\n",
      "# heading\n",
      "- a\n- b\n- c\n",
      "1. one\n2. two\n",
      "> quoted\nline\n",
      "```\ncode\n```\n",
      "    indented\n    code\n",
      "text **bold** _em_ `code`\n",
      "<details>\n<summary>Analysis</summary>\n\nbody\n</details>\n",
  };
  for (const auto& in : inputs) {
    for (size_t chunk : {1u, 2u, 3u, 5u, 8u, 64u}) {
      EXPECT_EQ(Regular(in), BlockStreamChunked(in, chunk))
          << "input: " << in << " chunk: " << chunk;
    }
  }
}

TEST(StreamingBlockParser, GfmOptionsMatchRegular) {
  struct Case {
    markus::Options options;
    std::string input;
  };
  const std::vector<Case> cases = {
      {markus::Options{.enable_tables = true},
       "| a | b |\n|---|---|\n| 1 | 2 |\n"},
      {markus::Options{.enable_strikethrough = true}, "~~gone~~\n"},
      {markus::Options{.enable_tasklist = true}, "- [x] done\n- [ ] todo\n"},
      {markus::Options{.enable_autolink = true},
       "visit https://example.com now\n"},
  };
  for (const auto& c : cases) {
    EXPECT_EQ(Regular(c.input, c.options),
              BlockStreamChunked(c.input, 3, c.options))
        << "input: " << c.input;
  }
}

// Each top-level block is delivered exactly once, in document order; the
// trailing block that may still grow is held back until Flush.
TEST(StreamingBlockParser, ProgressiveEmissionOrder) {
  const std::string input = "para1\n\npara2\n\npara3\n";
  const std::string expected = Regular(input);

  std::string running;
  size_t blocks_delivered = 0;
  bool trailing_held = false;
  {
    markus::StreamingBlockParser parser;
    parser.setBlockCallback([&](const markus::Document& doc, size_t first,
                                size_t last) {
      EXPECT_EQ(0u, first) << "emissions must be prefix ranges in order";
      blocks_delivered += last - first;
      running += RenderRange(doc, first, last);
      // Every prefix of the emitted output must itself be a prefix of the
      // final document (blocks are never revised or reordered).
      EXPECT_EQ(0u, expected.rfind(running, 0) == 0 ? 0u : 1u)
          << "emissions must stay a prefix of the final output; running: "
          << running;
    });
    parser.Feed("para1\n\n");
    EXPECT_EQ(1u, blocks_delivered);
    parser.Feed("para2\n\n");
    EXPECT_EQ(2u, blocks_delivered);
    // The trailing paragraph is still open: it must not be emitted yet.
    parser.Feed("para3\n");
    trailing_held = (blocks_delivered == 2);
    parser.Flush();
  }
  EXPECT_TRUE(trailing_held)
      << "the trailing (incomplete) paragraph must be held until Flush";
  EXPECT_EQ(3u, blocks_delivered);
  EXPECT_EQ(expected, running);
}

// An open fenced code block is held back until its closing fence arrives.
TEST(StreamingBlockParser, OpenCodeBlockHeldBack) {
  const std::string input = "before\n\n```\ncode\n```\nafter\n";
  size_t blocks_delivered = 0;
  std::string running;
  markus::StreamingBlockParser parser;
  parser.setBlockCallback([&](const markus::Document& doc, size_t first,
                              size_t last) {
    blocks_delivered += last - first;
    running += RenderRange(doc, first, last);
  });
  parser.Feed("before\n\n");
  EXPECT_EQ(1u, blocks_delivered);
  parser.Feed("```\ncode\n");  // fence still open
  EXPECT_EQ(1u, blocks_delivered) << "open code block must be held back";
  parser.Feed("```\nafter\n");
  EXPECT_EQ(2u, blocks_delivered) << "code block completes on its closing fence";
  parser.Flush();
  EXPECT_EQ(3u, blocks_delivered);
  EXPECT_EQ(Regular(input), running);
}

// An unclosed <details> section is held back until </details> (or Flush).
TEST(StreamingBlockParser, DetailsHeldBackUntilClosed) {
  const std::string open = "<details>\n<summary>s</summary>\nbody\n";
  const std::string input = open + "</details>\n";
  size_t blocks_delivered = 0;
  std::string running;
  markus::StreamingBlockParser parser;
  parser.setBlockCallback([&](const markus::Document& doc, size_t first,
                              size_t last) {
    blocks_delivered += last - first;
    running += RenderRange(doc, first, last);
  });
  parser.Feed(open);
  EXPECT_EQ(0u, blocks_delivered)
      << "an unclosed <details> section must be held back";
  parser.Feed("</details>\n");
  EXPECT_EQ(1u, blocks_delivered);
  parser.Flush();
  EXPECT_EQ(Regular(input), running);

  // An unclosed section at end-of-input is delivered (open) on Flush.
  std::string flush_out;
  size_t flush_blocks = 0;
  {
    markus::StreamingBlockParser p2;
    p2.setBlockCallback([&](const markus::Document& doc, size_t first,
                            size_t last) {
      flush_blocks += last - first;
      flush_out += RenderRange(doc, first, last);
    });
    p2.Feed(open);
    p2.Flush();
  }
  EXPECT_EQ(1u, flush_blocks);
  EXPECT_EQ(Regular(open), flush_out);
}

// A link reference defined in an earlier chunk resolves in a later one.
TEST(StreamingBlockParser, LinkRefDefinedBeforeUseResolves) {
  const std::string input = "[ref]: /url\n\n[ref]\n";
  EXPECT_EQ(Regular(input), BlockStreamChunked(input, 2));
  EXPECT_EQ(Regular(input), BlockStreamSingleFeed(input));
}

// Empty Feed() is a no-op; Reset() discards buffered content and references.
TEST(StreamingBlockParser, ResetAndEmptyFeed) {
  markus::StreamingBlockParser parser;
  std::string out;
  parser.setBlockCallback(
      [&](const markus::Document& doc, size_t first, size_t last) {
        out += RenderRange(doc, first, last);
      });

  parser.Feed("");  // no-op
  EXPECT_TRUE(parser.empty());
  parser.Flush();
  EXPECT_EQ(std::string(), out);

  parser.Feed("para1\n");
  parser.Reset();
  EXPECT_TRUE(parser.empty());
  parser.Feed("para2\n");
  parser.Flush();
  EXPECT_EQ(Regular("para2\n"), out)
      << "content before Reset() must be discarded; got: " << out;
}

// A Feed that would push the held-back buffer past setPendingLimit throws
// std::length_error and leaves the buffer unchanged.
TEST(StreamingBlockParser, PendingLimit) {
  markus::StreamingBlockParser small;
  small.setPendingLimit(4);
  EXPECT_THROW(small.Feed("12345\n"), std::length_error);
  EXPECT_TRUE(small.empty());

  markus::StreamingBlockParser big;
  big.setPendingLimit(1024);
  EXPECT_NO_THROW(big.Feed(std::string(1024, 'a')));
}

}  // namespace