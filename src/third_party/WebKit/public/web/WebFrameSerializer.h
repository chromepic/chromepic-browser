/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebFrameSerializer_h
#define WebFrameSerializer_h

#include "../platform/WebCString.h"
#include "../platform/WebCommon.h"
#include "../platform/WebData.h"
#include "../platform/WebString.h"
#include "../platform/WebURL.h"
#include "../platform/WebVector.h"

#include <utility>

//ChromePic
#include <vector> 
//ChromePic

namespace blink {

class WebFrameSerializerClient;
class WebFrame;
class WebLocalFrame;
template <typename T> class WebVector;

// Serialization of frame contents into html or mhtml.
class WebFrameSerializer {
public:
    // Generates and returns an MHTML header.
    //
    // Contents of the header (i.e. title and mime type) will be based
    // on the frame passed as an argument (which typically should be
    // the main, top-level frame).
    //
    // Same |boundary| needs to used for all generateMHTMLHeader and
    // generateMHTMLParts and generateMHTMLFooter calls that belong to the same
    // MHTML document (see also rfc1341, section 7.2.1, "boundary" description).
    BLINK_EXPORT static WebData generateMHTMLHeader(
        const WebString& boundary, WebLocalFrame*);

    // Delegate for controling the behavior of generateMHTMLParts method.
    class MHTMLPartsGenerationDelegate {
    public:
        // Tells whether to skip serialization of a subresource with a given URI.
        // Used to deduplicate resources across multiple frames.
        virtual bool shouldSkipResource(const WebURL&) = 0;

        // Returns a Content-ID to be used for the given frame.
        // See rfc2557 - section 8.3 - "Use of the Content-ID header and CID URLs".
        // Format note - the returned string should be of the form "<foo@bar.com>"
        // (i.e. the strings should include the angle brackets).  The method
        // should return null WebString if the frame doesn't have a content-id.
        virtual WebString getContentID(const WebFrame&) = 0;
    };

    // ChromePic
    BLINK_EXPORT static std::vector<WebData> generateMHTMLPartsForAllFrames(
        const WebString& boundary, WebLocalFrame*, bool useBinaryEncoding,
        MHTMLPartsGenerationDelegate*);
    // ChromePic


    // Generates and returns MHTML parts for the given frame and the
    // savable resources underneath.
    //
    // Same |boundary| needs to used for all generateMHTMLHeader and
    // generateMHTMLParts and generateMHTMLFooter calls that belong to the same
    // MHTML document (see also rfc1341, section 7.2.1, "boundary" description).
    BLINK_EXPORT static WebData generateMHTMLParts(
        const WebString& boundary, WebLocalFrame*, bool useBinaryEncoding,
        MHTMLPartsGenerationDelegate*);

    // Generates and returns an MHTML footer.
    //
    // Same |boundary| needs to used for all generateMHTMLHeader and
    // generateMHTMLParts and generateMHTMLFooter calls that belong to the same
    // MHTML document (see also rfc1341, section 7.2.1, "boundary" description).
    BLINK_EXPORT static WebData generateMHTMLFooter(const WebString& boundary);

    // IMPORTANT:
    // The API below is an older implementation of frame serialization that
    // will be removed soon.

    // This function will serialize the specified frame to HTML data.
    // We have a data buffer to temporary saving generated html data. We will
    // sequentially call WebFrameSerializerClient once the data buffer is full.
    //
    // Return false means if no data has been serialized (i.e. because
    // the target frame didn't have a valid url).
    //
    // The parameter frame specifies which frame need to be serialized.
    // The parameter client specifies the pointer of interface
    // WebFrameSerializerClient providing a sink interface to receive the
    // individual chunks of data to be saved.
    // The parameter urlsToLocalPaths contains a mapping between original URLs
    // of saved resources and corresponding local file paths.
    BLINK_EXPORT static bool serialize(
        WebLocalFrame*,
        WebFrameSerializerClient*,
        const WebVector<std::pair<WebURL, WebString>>& urlsToLocalPaths);

    // FIXME: The following are here for unit testing purposes. Consider
    // changing the unit tests instead.

    // Generate the META for charset declaration.
    BLINK_EXPORT static WebString generateMetaCharsetDeclaration(const WebString& charset);
    // Generate the MOTW declaration.
    BLINK_EXPORT static WebString generateMarkOfTheWebDeclaration(const WebURL&);
    // Generate the default base tag declaration.
    BLINK_EXPORT static WebString generateBaseTagDeclaration(const WebString& baseTarget);
};

} // namespace blink

#endif
