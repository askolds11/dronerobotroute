//
// Created by askolds on 5/25/25.
//

#ifndef STRIP_H
#define STRIP_H

/**
 *
 * @param originalImg Oriģinālais attēls
 * @param thresholdedImg Attēls ar krūma punktiem
 * @param mask Maska, kurā rēķināt punktus
 * @param turnaroundMask Maska, kurā var pārvietots starp rindām
 * @param stripHeight
 * @param threshold
 * @param tolerance
 * @param previousTolerance
 * @param workDir Work dir
 * @param nr
 * @return
 */
const std::string Strip(
    const char* originalImg,
    const char* thresholdedImg,
    const char* mask,
    const char* turnaroundMask,
    int stripHeight,
    float threshold,
    float tolerance,
    float previousTolerance,
    const char* workDir,
    int nr);

#endif //STRIP_H
