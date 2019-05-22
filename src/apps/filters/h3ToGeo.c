/*
 * Copyright 2016-2017, 2019 Uber Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/** @file
 * @brief stdin/stdout filter that converts from integer H3 indexes to lat/lon
 * cell center point
 *
 *  See `h3ToGeo --help` for usage.
 *
 *  The program reads H3 indexes from stdin and outputs the corresponding
 *  cell center points to stdout, until EOF is encountered. The H3 indexes
 *  should be in integer form.
 *
 *  `--kml` causes KML output to be printed. `--kml-name` and
 *  `--kml-description` can be used to change the name and description in the
 *  KML header, which default to "H3 Geometry" and "Generated by h3ToGeo"
 *  respectively.
 *
 *  Examples:
 *
 *     `h3ToGeo < indexes.txt`
 *        - outputs plain text cell center points for the H3 indexes contained
 *          in the file `indexes.txt`
 *
 *     `h3ToGeo --kml --kml-name "kml title" --kml-description "h3 cells" <
 *          indexes.txt > cells.kml`
 *        - creates the KML file `cells.kml` containing the cell center points
 *          for all of the H3 indexes contained in the file `indexes.txt`.
 */

#include <inttypes.h>
#include "args.h"
#include "h3api.h"
#include "kml.h"
#include "utility.h"

void doCell(H3Index h, int isKmlOut) {
    GeoCoord g;
    H3_EXPORT(h3ToGeo)(h, &g);

    char label[BUFF_SIZE];

    if (isKmlOut) {
        H3_EXPORT(h3ToString)(h, label, BUFF_SIZE);

        outputPointKML(&g, label);
    } else {
        printf("%.10lf %.10lf\n", H3_EXPORT(radsToDegs)(g.lat),
               H3_EXPORT(radsToDegs)(g.lon));
    }
}

int main(int argc, char *argv[]) {
    H3Index index = 0;

    Arg helpArg = ARG_HELP;
    Arg indexArg = {
        .names = {"-i", "--index"},
        .scanFormat = "%" PRIx64,
        .valueName = "index",
        .value = &index,
        .helpText =
            "Index, or not specified to read indexes from standard input."};
    Arg kmlArg = ARG_KML;
    DEFINE_KML_NAME_ARG(userKmlName, kmlNameArg);
    DEFINE_KML_DESC_ARG(userKmlDesc, kmlDescArg);

    Arg *args[] = {&helpArg, &indexArg, &kmlArg, &kmlNameArg, &kmlDescArg};

    if (parseArgs(argc, argv, 5, args, &helpArg,
                  "Converts indexes to latitude/longitude center coordinates "
                  "in degrees")) {
        return helpArg.found ? 0 : 1;
    }

    if (kmlArg.found) {
        char *kmlName = "H3 Geometry";
        if (kmlNameArg.found) kmlName = userKmlName;

        char *kmlDesc = "Generated by h3ToGeo";
        if (kmlDescArg.found) kmlDesc = userKmlDesc;

        kmlPtsHeader(kmlName, kmlDesc);
    }

    if (indexArg.found) {
        doCell(index, kmlArg.found);
    } else {
        // process the indexes on stdin
        char buff[BUFF_SIZE];
        while (1) {
            // get an index from stdin
            if (!fgets(buff, BUFF_SIZE, stdin)) {
                if (feof(stdin))
                    break;
                else
                    error("reading H3 index from stdin");
            }

            H3Index h3 = H3_EXPORT(stringToH3)(buff);
            doCell(h3, kmlArg.found);
        }
    }

    if (kmlArg.found) kmlPtsFooter();
}
