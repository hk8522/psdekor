/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */
/*
 *      PROJECT:   ASxxxx firmware
 *      $Revision: $
 *      LANGUAGE:  ANSI C
 */

/*! \file
 *
 *  \author M. Arpa
 *  \author Manuel Huber (AVR port)
 *
 *  \brief Main error codes. Please add your application specific
 *  error codes in your application starting with
 *  \#define MY_APP_ERR_CODE (ERR_LAST_ERROR-1)
 *
 *  \note Platform specific error codes (that have already been defined by Atmel) are
 *        used (values).
 */

#ifndef ERRNO_H
#define ERRNO_H

/*!
 * Error codes to be used within the application.
 * They are represented by an s8
 */
#define ERR_NONE    STATUS_OK
#define ERR_NOMEM   ERR_NO_MEMORY
#define ERR_IO      ERR_IO_ERROR
#define ERR_REQUEST -15 /*!< invalid request or requested function can't be executed at the moment */
#define ERR_NOMSG   -16 /*!< No message of desired type */
#define ERR_PARAM   ERR_INVALID_ARG /*!< Parameter error */

#define ERR_LAST_ERROR -32

#endif /* ERRNO_H */

